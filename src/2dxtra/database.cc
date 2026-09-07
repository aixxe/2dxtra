#include "database.h"

#include <cstring>
#include <sqlite3.h>
#include <zstd.h>

namespace database
{
    struct db
    {
        sqlite3* handle = nullptr;
        sqlite3_stmt* stmt_lookup = nullptr;
        sqlite3_stmt* stmt_insert = nullptr;
        sqlite3_stmt* stmt_insert_orig = nullptr;
        sqlite3_stmt* stmt_delete_orig = nullptr;
        sqlite3_stmt* stmt_pull = nullptr;
        sqlite3_stmt* stmt_list_sets = nullptr;
        sqlite3_stmt* stmt_charts_for_set = nullptr;
        sqlite3_stmt* stmt_exists = nullptr;
        sqlite3_stmt* stmt_count = nullptr;
    };

    namespace
    {
        constexpr char kCreate[] =
            "CREATE TABLE IF NOT EXISTS chart_set ("
            "  id   INTEGER PRIMARY KEY,"
            "  name TEXT NOT NULL UNIQUE"
            ");"
            "CREATE TABLE IF NOT EXISTS original_charts ("
            "  music_id   INTEGER NOT NULL,"
            "  difficulty INTEGER NOT NULL,"
            "  hash       TEXT    NOT NULL,"
            "  PRIMARY KEY (music_id, difficulty)"
            ");"
            "CREATE TABLE IF NOT EXISTS charts ("
            "  chart_set     INTEGER NOT NULL REFERENCES chart_set(id),"
            "  music_id      INTEGER NOT NULL,"
            "  difficulty    INTEGER NOT NULL,"
            "  hash          TEXT    NOT NULL,"
            "  notes         INTEGER NOT NULL,"
            "  radar_notes   INTEGER, radar_peak INTEGER, radar_scratch INTEGER,"
            "  radar_soflan  INTEGER, radar_charge INTEGER, radar_chord INTEGER,"
            "  data          BLOB    NOT NULL,"
            "  PRIMARY KEY (chart_set, music_id, difficulty),"
            "  FOREIGN KEY (music_id, difficulty)"
            "    REFERENCES original_charts(music_id, difficulty)"
            "    ON DELETE CASCADE"
            ");";

        constexpr char kSeedChartSets[] =
            "INSERT OR IGNORE INTO chart_set (id, name)"
            " VALUES (0, 'Kiraku'), (1, 'Kichiku'), (2, 'All-Scratch');";

        constexpr char kPragma[] =
            "PRAGMA journal_mode=WAL;"
            "PRAGMA synchronous=NORMAL;"
            "PRAGMA foreign_keys=ON;";

        constexpr char kPragmaDelete[] =
            "PRAGMA journal_mode=DELETE;";

        constexpr char kLookup[] =
            "SELECT c.hash, c.notes,"
            "  c.radar_notes, c.radar_peak, c.radar_scratch,"
            "  c.radar_soflan, c.radar_charge, c.radar_chord, o.hash"
            " FROM charts c JOIN original_charts o"
            "   ON o.music_id = c.music_id AND o.difficulty = c.difficulty"
            " WHERE c.chart_set=?1 AND c.music_id=?2 AND c.difficulty=?3;";

        constexpr char kDeleteOrig[] =
            "DELETE FROM original_charts"
            " WHERE music_id=?1 AND difficulty=?2;";

        constexpr char kInsert[] =
            "INSERT OR REPLACE INTO charts"
            " (chart_set, music_id, difficulty, hash, notes,"
            "  radar_notes, radar_peak, radar_scratch, radar_soflan,"
            "  radar_charge, radar_chord, data)"
            " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12);";

        constexpr char kInsertOrig[] =
            "INSERT OR IGNORE INTO original_charts (music_id, difficulty, hash)"
            " VALUES (?1, ?2, ?3);";

        constexpr char kPull[] =
            "SELECT hash, notes, data FROM charts"
            " WHERE chart_set=?1 AND music_id=?2 AND difficulty=?3;";

        constexpr char kListSets[] =
            "SELECT id, name FROM chart_set ORDER BY id;";

        constexpr char kChartsForSet[] =
            "SELECT music_id, difficulty, hash, notes,"
            "  radar_notes, radar_peak, radar_scratch,"
            "  radar_soflan, radar_charge, radar_chord"
            " FROM charts WHERE chart_set=?1 ORDER BY music_id, difficulty;";

        constexpr char kExists[] =
            "SELECT 1 FROM charts WHERE hash=?1 LIMIT 1;";

        constexpr char kCount[] =
            "SELECT COUNT(*) FROM charts;";

        auto bind_key(sqlite3_stmt* stmt, int chart_set, int music_id,
                      int difficulty) -> void
        {
            sqlite3_bind_int(stmt, 1, chart_set);
            sqlite3_bind_int(stmt, 2, music_id);
            sqlite3_bind_int(stmt, 3, difficulty);
        }

        auto bind_music_key(sqlite3_stmt* stmt, int music_id, int difficulty)
            -> void
        {
            sqlite3_bind_int(stmt, 1, music_id);
            sqlite3_bind_int(stmt, 2, difficulty);
        }

        auto read_radar(sqlite3_stmt* stmt, int base_col, int out[6]) -> void
        {
            for (int i = 0; i < 6; ++i)
                out[i] = sqlite3_column_int(stmt, base_col + i);
        }

        auto bind_radar(sqlite3_stmt* stmt, int base_param, const int radar[6])
            -> void
        {
            for (int i = 0; i < 6; ++i)
                sqlite3_bind_int(stmt, base_param + i, radar[i]);
        }

        auto read_row(sqlite3_stmt* stmt) -> std::optional<chart_row>
        {
            if (sqlite3_step(stmt) != SQLITE_ROW)
                return std::nullopt;

            chart_row row;
            row.hash  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            row.notes = sqlite3_column_int(stmt, 1);
            read_radar(stmt, 2, row.radar);
            return row;
        }

        constexpr int ZSTD_LEVEL = 3;

        auto compress_blob(const std::vector<std::uint8_t>& data)
            -> std::vector<std::uint8_t>
        {
            if (data.empty())
                return {};

            auto const bound = ZSTD_compressBound(data.size());
            auto out = std::vector<std::uint8_t>(bound);
            auto const n = ZSTD_compress(
                out.data(), bound, data.data(), data.size(), ZSTD_LEVEL);
            if (ZSTD_isError(n))
                return {};

            out.resize(n);
            return out;
        }

        auto decompress_into(const std::uint8_t* src, std::size_t src_size,
                             std::uint8_t* dst, std::size_t capacity)
            -> std::size_t
        {
            if (src == nullptr || src_size == 0 || dst == nullptr)
                return 0;

            auto const content = ZSTD_getFrameContentSize(src, src_size);
            if (content == ZSTD_CONTENTSIZE_ERROR ||
                content == ZSTD_CONTENTSIZE_UNKNOWN ||
                content > capacity)
                return 0;

            auto const n = ZSTD_decompress(dst, capacity, src, src_size);
            return ZSTD_isError(n) ? 0 : n;
        }
    }

    auto open(const char* path) -> db*
    {
        auto* d = new db;
        if (sqlite3_open(path, &d->handle) != SQLITE_OK)
        {
            delete d;
            return nullptr;
        }
        sqlite3_exec(d->handle, kPragma, nullptr, nullptr, nullptr);

        if (sqlite3_exec(d->handle, kCreate, nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            sqlite3_close(d->handle);
            delete d;
            return nullptr;
        }
        sqlite3_exec(d->handle, kSeedChartSets, nullptr, nullptr, nullptr);

        sqlite3_prepare_v2(d->handle, kLookup, -1, &d->stmt_lookup, nullptr);        sqlite3_prepare_v2(d->handle, kInsert,  -1, &d->stmt_insert, nullptr);
        sqlite3_prepare_v2(d->handle, kInsertOrig, -1, &d->stmt_insert_orig, nullptr);
        sqlite3_prepare_v2(d->handle, kDeleteOrig, -1, &d->stmt_delete_orig, nullptr);
        sqlite3_prepare_v2(d->handle, kPull, -1, &d->stmt_pull, nullptr);
        sqlite3_prepare_v2(d->handle, kListSets, -1, &d->stmt_list_sets, nullptr);
        sqlite3_prepare_v2(d->handle, kChartsForSet, -1, &d->stmt_charts_for_set, nullptr);
        sqlite3_prepare_v2(d->handle, kExists, -1, &d->stmt_exists, nullptr);
        sqlite3_prepare_v2(d->handle, kCount, -1, &d->stmt_count, nullptr);
        return d;
    }

    void close(db* d)
    {
        if (!d) return;
        sqlite3_finalize(d->stmt_lookup);
        sqlite3_finalize(d->stmt_insert);
        sqlite3_finalize(d->stmt_insert_orig);
        sqlite3_finalize(d->stmt_delete_orig);
        sqlite3_finalize(d->stmt_pull);
        sqlite3_finalize(d->stmt_list_sets);
        sqlite3_finalize(d->stmt_charts_for_set);
        sqlite3_finalize(d->stmt_exists);
        sqlite3_finalize(d->stmt_count);
        sqlite3_close(d->handle);
        delete d;
    }

    auto lookup(db* d, int chart_set, int music_id, int difficulty,
                const std::string& orig_hash) -> std::optional<chart_row>
    {
        if (!d) return std::nullopt;

        auto* stmt = d->stmt_lookup;
        sqlite3_reset(stmt);
        bind_key(stmt, chart_set, music_id, difficulty);

        auto row = read_row(stmt);
        if (!row)
        {
            sqlite3_reset(stmt);
            return std::nullopt;
        }

        auto const stored = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 8));
        if (stored != orig_hash)
        {
            sqlite3_reset(stmt);
            auto* del = d->stmt_delete_orig;
            sqlite3_reset(del);
            bind_music_key(del, music_id, difficulty);
            sqlite3_step(del);
            sqlite3_reset(del);
            return std::nullopt;
        }

        row->chart_set   = chart_set;
        row->music_id    = music_id;
        row->difficulty  = difficulty;
        sqlite3_reset(stmt);
        return row;
    }

    void insert(db* d, const chart_row& row, const std::string& orig_hash)
    {
        if (!d) return;

        auto* orig = d->stmt_insert_orig;
        sqlite3_reset(orig);
        bind_music_key(orig, row.music_id, row.difficulty);
        sqlite3_bind_text(orig, 3, orig_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(orig);

        auto* stmt = d->stmt_insert;
        sqlite3_reset(stmt);
        bind_key(stmt, row.chart_set, row.music_id, row.difficulty);
        sqlite3_bind_text(stmt, 4, row.hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, static_cast<int>(row.notes));
        bind_radar(stmt, 6, row.radar);

        auto const compressed = compress_blob(row.data);
        auto const* blob = compressed.empty()
            ? reinterpret_cast<const std::uint8_t*>("")
            : compressed.data();
        sqlite3_bind_blob(stmt, 12, blob,
            static_cast<int>(compressed.size()), SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }

    auto pull(db* d, int chart_set, int music_id, int difficulty,
              std::uint8_t* dst, std::size_t capacity)
        -> std::optional<pulled_chart>
    {
        if (!d || dst == nullptr)
            return std::nullopt;

        auto* stmt = d->stmt_pull;
        sqlite3_reset(stmt);
        bind_key(stmt, chart_set, music_id, difficulty);

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_reset(stmt);
            return std::nullopt;
        }

        auto const* blob = static_cast<const std::uint8_t*>(
            sqlite3_column_blob(stmt, 2));
        auto const size = sqlite3_column_bytes(stmt, 2);
        if (blob == nullptr || size == 0)
        {
            sqlite3_reset(stmt);
            return std::nullopt;
        }

        auto const written = decompress_into(blob, static_cast<std::size_t>(size),
            dst, capacity);
        if (written == 0)
        {
            sqlite3_reset(stmt);
            return std::nullopt;
        }

        auto result = pulled_chart {
            .hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            .notes = static_cast<std::uint32_t>(sqlite3_column_int(stmt, 1)),
        };
        sqlite3_reset(stmt);
        return result;
    }

    auto list_sets(db* d) -> std::vector<std::pair<int, std::string>>
    {
        auto result = std::vector<std::pair<int, std::string>> {};
        if (!d) return result;

        auto* stmt = d->stmt_list_sets;
        sqlite3_reset(stmt);

        while (sqlite3_step(stmt) == SQLITE_ROW)
            result.emplace_back(sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));

        sqlite3_reset(stmt);
        return result;
    }

    auto charts_for_set(db* d, int chart_set) -> std::vector<chart_row>
    {
        auto result = std::vector<chart_row> {};
        if (!d) return result;

        auto* stmt = d->stmt_charts_for_set;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, chart_set);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            chart_row row;
            row.music_id   = sqlite3_column_int(stmt, 0);
            row.difficulty = sqlite3_column_int(stmt, 1);
            row.hash       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            row.notes      = sqlite3_column_int(stmt, 3);
            read_radar(stmt, 4, row.radar);
            result.push_back(std::move(row));
        }

        sqlite3_reset(stmt);
        return result;
    }

    auto exists(db* d, const std::string& hash) -> bool
    {
        if (!d) return false;

        auto* stmt = d->stmt_exists;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
        auto const found = sqlite3_step(stmt) == SQLITE_ROW;
        sqlite3_reset(stmt);
        return found;
    }

    auto chart_count(db* d) -> std::size_t
    {
        if (!d) return 0;

        auto* stmt = d->stmt_count;
        sqlite3_reset(stmt);
        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_reset(stmt);
            return 0;
        }
        auto const count = static_cast<std::size_t>(sqlite3_column_int64(stmt, 0));
        sqlite3_reset(stmt);
        return count;
    }

    void flush(db* d)
    {
        if (!d) return;

        sqlite3_reset(d->stmt_lookup);
        sqlite3_reset(d->stmt_insert);
        sqlite3_reset(d->stmt_insert_orig);
        sqlite3_reset(d->stmt_delete_orig);
        sqlite3_reset(d->stmt_pull);
        sqlite3_reset(d->stmt_list_sets);
        sqlite3_reset(d->stmt_charts_for_set);
        sqlite3_reset(d->stmt_exists);
        sqlite3_reset(d->stmt_count);

        sqlite3_wal_checkpoint_v2(
            d->handle, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
        sqlite3_exec(d->handle, kPragmaDelete, nullptr, nullptr, nullptr);
    }
}
