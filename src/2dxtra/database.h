#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace database
{
    struct chart_row
    {
        int chart_set; // 0=Kiraku, 1=Kichiku
        int music_id;
        int difficulty; // 0..9 (loader chart index)
        std::string hash; // sha256 of the mutated chart (also its id)
        std::uint32_t notes; // note count of the mutated chart
        int radar[6]; // notes, peak, scratch, soflan, charge, chord
        std::vector<std::uint8_t> data;  // mutated chart binary
    };

    struct pulled_chart
    {
        std::string   hash;
        std::uint32_t notes;
    };

    struct db;

    auto open(const char* path) -> db*;
    auto close(db*) -> void;
    auto lookup(db*, int chart_set, int music_id, int difficulty, const std::string& orig_hash) -> std::optional<chart_row>;
    auto insert(db*, const chart_row& row, const std::string& orig_hash) -> void;
    auto pull(db*, int chart_set, int music_id, int difficulty, std::uint8_t* dst, std::size_t capacity) -> std::optional<pulled_chart>;
    auto list_sets(db*) -> std::vector<std::pair<int, std::string>>;
    auto charts_for_set(db*, int chart_set) -> std::vector<chart_row>;
    auto exists(db*, const std::string& hash) -> bool;
    auto chart_count(db*) -> std::size_t;
    void flush(db*);
}
