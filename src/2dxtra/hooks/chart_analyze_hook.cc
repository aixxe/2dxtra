#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <safetyhook.hpp>

#include "../log.h"
#include "../game.h"
#include "../database.h"
#include "../urafumen.h"
#include "../all_scratch.h"
#include "chart_analyze_hook.h"

namespace iidxtra::chart_analyze_hook
{
    namespace
    {
        constexpr std::size_t SCRATCH_CAPACITY = bm2dx::CHART_BUFFER_BYTES;
        constexpr std::size_t DIFF_STRIDE = 0x230;
        constexpr std::size_t DIFF_COUNT = 10;
        constexpr std::size_t V22_SIZE = DIFF_STRIDE * DIFF_COUNT;
        constexpr std::size_t NOTE_OFFSET_IN_REC = 4 + 8;

        // The 10 analyze-gate bytes are the contiguous u8 rating fields of
        // music_entry_t (SPB..SPL, padding, DPN..DPL); index 5 is the padding
        // byte, always zero (invalid).
        auto inline gate_rating(const bm2dx::music_entry_t* entry) -> const std::uint8_t*
            { return reinterpret_cast<const std::uint8_t*>(&entry->spb_rating); }

        using radar_fn = auto (__fastcall*)(int*, void*) -> void*;

        SafetyHookInline g_analyze_hook {};
        std::unique_ptr<SafetyHookMid> g_read_hook {};
        SafetyHookInline g_init_text_hook {};
        radar_fn g_radar = nullptr;
        database::db* g_db = nullptr;

        bool g_needs_build = false;

        // Boot debug text renderer (sub_1805E0250): (size, x, y, style, text).
        auto init_text_render_detour(int a1, int x, int y, void* style, const char* text) -> void
        {
            // Replace only the actual "Initialize Game" slot (sub_180958020
            // renders it at (60, 80)); other boot texts must be left alone.
            if (g_needs_build && text && std::string_view { text }.starts_with("Initialize Game"))
                text = "Building chart sets - this may take a few minutes...";
            return g_init_text_hook.call<void>(a1, x, y, style, text);
        }

        bool g_mutate = false;
        bool g_pass2 = false;
        int  g_mutate_mode = 0;

        std::vector<std::string> g_orig_hashes {};
        std::vector<std::string> g_mutated_hashes {};
        std::vector<std::vector<std::uint8_t>> g_mutated_data {};

        auto read_after_hook(SafetyHookContext& ctx) -> void
        {
            auto const read = static_cast<std::size_t>(ctx.rax);
            if (read == 0)
                return;

            auto* const buf = reinterpret_cast<std::uint8_t*>(ctx.rdi);

            // Hash only the bytes the AVS read returned, so the hash matches
            // the raw chart data (and the mod's offline tooling). The game
            // caps reads at SCRATCH_CAPACITY, so `read` cannot overrun.
            if (!g_mutate)
                g_orig_hashes.push_back(urafumen::sha256_hex(buf, read));

            if (!g_mutate)
                return;

            auto const written = g_mutate_mode == 2
                ? all_scratch::convert_in_place(buf, SCRATCH_CAPACITY)
                : urafumen::convert_in_place(
                    buf, SCRATCH_CAPACITY, /*player=*/0, g_mutate_mode == 1);

            if (written > 0)
            {
                // Preserve the source stream's tail past the converted chart
                // (the 0x7FFFFFFF sentinel and any trailing events) so the
                // stored blob matches the reference export when the converted
                // chart is shorter than the source.
                auto const stored = std::max(written, read);
                g_mutated_hashes.push_back(
                    urafumen::sha256_hex(buf, stored));
                g_mutated_data.emplace_back(buf, buf + stored);
            }
            else
            {
                // Conversion failed: keep a blank slot so the rank-indexed
                // vectors stay aligned (see hash_for_diff / data_for_diff).
                g_mutated_hashes.push_back({});
                g_mutated_data.emplace_back();
            }
        }

        auto note_count(const std::uint8_t* v22, int d) -> int
        {
            auto const off = NOTE_OFFSET_IN_REC + DIFF_STRIDE * d;
            return *reinterpret_cast<const int*>(v22 + off);
        }

        auto radar_of(const std::uint8_t* v22, int d, std::int32_t out[6]) -> void
        {
            auto const rec = const_cast<int*>(
                reinterpret_cast<const int*>(v22 + DIFF_STRIDE * d));
            std::uint8_t tmp[24] {};
            g_radar(rec, tmp);
            std::memcpy(out, tmp, sizeof(std::int32_t) * 6);
        }

        auto is_valid_diff(std::uint8_t gate_byte) -> bool
        {
            return gate_byte >= 1 && gate_byte <= 0xC;
        }

        auto diff_rank(const bm2dx::music_entry_t* entry, int d) -> int
        {
            int rank = 0;
            for (int i = 0; i < d; ++i)
                if (is_valid_diff(gate_rating(entry)[i]))
                    ++rank;
            return rank;
        }

        auto hash_for_diff(const std::vector<std::string>& hashes,
                           const bm2dx::music_entry_t* entry, int d) -> std::string const&
        {
            static std::string const empty;
            if (d < 0 || d >= static_cast<int>(DIFF_COUNT) || entry == nullptr)
                return empty;
            if (!is_valid_diff(gate_rating(entry)[d]))
                return empty;
            auto const rank = diff_rank(entry, d);
            if (rank < 0 || rank >= static_cast<int>(hashes.size()))
                return empty;
            return hashes[rank];
        }

        auto data_for_diff(const std::vector<std::vector<std::uint8_t>>& blobs,
                           const bm2dx::music_entry_t* entry, int d)
            -> std::vector<std::uint8_t> const*
        {
            static std::vector<std::uint8_t> const empty;
            if (d < 0 || d >= static_cast<int>(DIFF_COUNT) || entry == nullptr)
                return &empty;
            if (!is_valid_diff(gate_rating(entry)[d]))
                return &empty;
            auto const rank = diff_rank(entry, d);
            if (rank < 0 || rank >= static_cast<int>(blobs.size()))
                return &empty;
            return &blobs[rank];
        }

        auto run_pass2(void* scratch, const bm2dx::music_entry_t* entry,
                       void* dummy_v22, void* flag, int mode) -> bool
        {
            auto dummy_rec = std::vector<std::uint8_t>(sizeof(bm2dx::music_entry_t), 0);
            std::memcpy(dummy_rec.data(), entry, sizeof(bm2dx::music_entry_t));

            g_mutate_mode = mode;
            g_pass2 = true;
            g_mutate = true;
            g_mutated_hashes.clear();
            g_mutated_data.clear();
            g_analyze_hook.call<__int64>(
                scratch, dummy_rec.data(), dummy_v22, flag);
            g_mutate = false;
            g_pass2 = false;

            return !g_mutated_hashes.empty();
        }

        auto analyze_detour(void* scratch, bm2dx::music_entry_t* entry,
                            void* v22, void* flag) -> __int64
        {
            if (g_pass2)
                return g_analyze_hook.call<__int64>(scratch, entry, v22, flag);

            g_mutate = false;
            g_orig_hashes.clear();
            auto const result = g_analyze_hook.call<__int64>(
                scratch, entry, v22, flag);

            auto const id = entry->id;

            struct diff_info { int d; int rank; std::string orig_hash; };
            std::vector<diff_info> valid_diffs;
            for (int d = 0; d < static_cast<int>(DIFF_COUNT); ++d)
            {
                if (!is_valid_diff(gate_rating(entry)[d]))
                    continue;
                auto const rank = diff_rank(entry, d);
                auto const& oh = hash_for_diff(g_orig_hashes, entry, d);
                if (oh.empty())
                    continue;
                valid_diffs.push_back({ d, rank, oh });
            }

            for (int mode = 0; mode < 3; ++mode)
            {
                std::vector<database::chart_row> cached(DIFF_COUNT);
                std::vector<bool> is_cached(DIFF_COUNT, false);
                int cache_hit_count = 0;

                for (auto const& di : valid_diffs)
                {
                    auto row = database::lookup(g_db, mode, id, di.d, di.orig_hash);
                    if (row)
                    {
                        cached[di.d] = std::move(*row);
                        is_cached[di.d] = true;
                        ++cache_hit_count;
                    }
                }

                bool const all_cached =
                    cache_hit_count == static_cast<int>(valid_diffs.size());

                if (!all_cached)
                {
                    auto dummy_v22 = std::vector<std::uint8_t>(V22_SIZE, 0);
                    run_pass2(scratch, entry, dummy_v22.data(), flag, mode);

                    for (auto const& di : valid_diffs)
                    {
                        if (is_cached[di.d])
                            continue;

                        auto const alt = note_count(dummy_v22.data(), di.d);
                        std::int32_t radar[6] {};
                        radar_of(dummy_v22.data(), di.d, radar);

                        auto const& mh = hash_for_diff(g_mutated_hashes, entry, di.d);
                        auto const* data = data_for_diff(g_mutated_data, entry, di.d);

                        database::chart_row row;
                        row.chart_set  = mode;
                        row.music_id   = id;
                        row.difficulty = di.d;
                        row.hash       = mh;
                        row.notes      = static_cast<std::uint32_t>(alt);
                        std::memcpy(row.radar, radar, sizeof(radar));
                        if (data)
                            row.data = *data;
                        database::insert(g_db, row, di.orig_hash);

                        cached[di.d] = std::move(row);
                        is_cached[di.d] = true;
                    }
                }
            }

            return result;
        }

        auto install_read_hook() -> void
        {
            auto const target = bm2dx::addr->CHART_ANALYZE_RESULT;
            auto hook_result = SafetyHookMid::create(
                reinterpret_cast<void*>(target), &read_after_hook);
            if (!hook_result.has_value())
            {
                log::print("analyze: failed to install mid-function read hook");
                return;
            }
            g_read_hook = std::make_unique<SafetyHookMid>(std::move(*hook_result));
        }

        auto install_analyze_hook() -> void
        {
            g_radar = reinterpret_cast<radar_fn>(bm2dx::addr->CHART_CALC_RADAR_FN);

            g_analyze_hook = safetyhook::create_inline(
                bm2dx::addr->CHART_ANALYZE_FN,
                reinterpret_cast<void*>(&analyze_detour));
            if (!g_analyze_hook)
            {
                log::print("analyze: failed to hook chart analyzer");
                return;
            }
        }
    }

    auto install(database::db* cache) -> void
    {
        g_db = cache;
        if (!g_db)
            return;

        // An empty chart cache means this boot performs the full (slow) build.
        g_needs_build = database::chart_count(g_db) == 0;
        if (g_needs_build)
            g_init_text_hook = safetyhook::create_inline(
                bm2dx::addr->INIT_TEXT_RENDER_FN,
                reinterpret_cast<void*>(&init_text_render_detour));

        install_analyze_hook();
        install_read_hook();
    }
}
