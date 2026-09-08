#include <cmath>
#include <mutex>
#include "../judgment.h"
#include "timing_histogram.h"

namespace iidxtra::timing_histogram
{
    bool enabled = false;

    auto state_mutex = std::mutex {};
    bool collecting = false;
    bool capture_enabled = false;
    auto current = result_t {};
    auto completed = completed_result_t {};

    auto timing_tracker_t::add(const float milliseconds, const judgment_t judgment) -> void
    {
        const auto category = static_cast<std::size_t>(judgment);

        // Sanity check for range
        if (category >= judgment_count ||
            !std::isfinite(milliseconds) ||
            std::abs(milliseconds) > 999.0f)
        {
            return;
        }

        ++count;
        sum += milliseconds;
        sum_squares += static_cast<double>(milliseconds) * milliseconds;

        const auto index =
            static_cast<int>(std::floor(milliseconds / bin_width_ms + 0.5f)) + bin_count / 2;

        if (std::abs(milliseconds) <= range_ms &&
            index >= 0 &&
            index < bin_count)
        {
            ++judgments[index][category];
        }
    }

    auto timing_tracker_t::mean() const -> double
    {
        return count == 0 ? 0.0 : sum / count;
    }

    auto timing_tracker_t::standard_deviation() const -> double
    {
        if (count == 0)
            return 0.0;

        const auto average = mean();
        const auto variance = sum_squares / count - average * average;
        return std::sqrt(variance > 0.0 ? variance : 0.0);
    }

    auto update() -> void
    {
        const auto lock = std::lock_guard { state_mutex };
        capture_enabled = enabled;
        collecting = false;

        current = {};
        completed = {};
    }

    auto reset() -> void
    {
        enabled = false;
        update();
    }

    auto begin_record(const bool is_dp) -> void
    {
        const auto lock = std::lock_guard { state_mutex };

        current = {};
        current.is_dp = is_dp;

        completed = {};
        collecting = capture_enabled;
    }

    auto is_recording() -> bool
    {
        const auto lock = std::lock_guard { state_mutex };
        return collecting;
    }

    auto record_note(const int player, const bool scratch, const float milliseconds, const int display_code) -> void
    {
        judgment_t judgment;
        switch (display_code)
        {
            // pgreat
            case bm2dx::judge_display_code::pgreat:
                judgment = judgment_t::pgreat;
                break;

            // early / late great
            case bm2dx::judge_display_code::early_great:
            case bm2dx::judge_display_code::late_great:
                judgment = judgment_t::great;
                break;

            // early / late good
            case bm2dx::judge_display_code::early_good:
            case bm2dx::judge_display_code::late_good:
                judgment = judgment_t::good;
                break;

            default:
                return;
        }

        const auto lock = std::lock_guard { state_mutex };
        if (!collecting || player < 0 || player >= 2)
            return;

        auto& result = current.players[current.is_dp ? 0 : player];
        auto& distribution = scratch ? result.scratch : result.keys;
        distribution.add(milliseconds, judgment);
    }

    auto stop_record() -> void
    {
        const auto lock = std::lock_guard { state_mutex };
        if (!collecting)
            return;

        completed = {current, capture_enabled};
        collecting = false;
    }

    auto leave_result() -> void
    {
        const auto lock = std::lock_guard { state_mutex };
        completed.visible = false;
    }

    auto snapshot() -> completed_result_t
    {
        const auto lock = std::lock_guard { state_mutex };
        return completed;
    }
}