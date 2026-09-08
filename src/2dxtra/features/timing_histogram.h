#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace iidxtra::timing_histogram
{

    // Assumes 120Hz ticks for TDJ; LDJ will fill every other bin.
    constexpr auto bin_width_ms = 1000.0f / 120.0f;

    // Covers -200ms to +200ms @ 120Hz ticks
    constexpr auto bin_count = 49;

    // For keys, graph spans -120ms to +120ms.
    // (wide enough for GOOD judgment)
    constexpr auto keys_range_ms = 120.0f;

    // For scratches, graph spans -200ms to +200ms
    // (wide enough for GOOD judgment on DP)
    constexpr auto scratch_range_ms = 200.0f;

    enum class judgment_t
    {
        pgreat,
        great,
        good,
        count
    };

    constexpr auto judgment_count = static_cast<std::size_t>(judgment_t::count);

    struct timing_tracker_t
    {
        // Range that we care about; [-range_ms, +range_ms]
        float range_ms = 0;

        // Bins counting number of hits, also separated per judgment type
        std::array<std::array<std::uint32_t, judgment_count>, bin_count> judgments {};

        // Total hits
        std::uint32_t count = 0;

        // Running total of all values
        double sum = 0.0;

        // Running total of squares of all values
        double sum_squares = 0.0;

        auto add(float milliseconds, judgment_t judgment) -> void;
        auto mean() const -> double;
        auto standard_deviation() const -> double;
    };

    struct player_result_t
    {
        timing_tracker_t keys {keys_range_ms};
        timing_tracker_t scratch {scratch_range_ms};
    };

    struct result_t
    {
        std::array<player_result_t, 2> players {};
        bool is_dp = false;
    };

    struct completed_result_t : result_t
    {
        bool visible = false;
    };

    extern bool enabled;

    auto update() -> void;
    auto reset() -> void;
    auto begin_record(bool is_dp = false) -> void;
    auto is_recording() -> bool;
    auto record_note(int player, bool scratch, float milliseconds, int display_code) -> void;
    auto stop_record() -> void;
    auto leave_result() -> void;
    auto snapshot() -> completed_result_t;
}