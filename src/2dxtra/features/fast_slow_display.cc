#include <fmt/format.h>
#include <algorithm>
#include <cmath>
#include "../hooks/fast_slow_hook.h"
#include "../judgment.h"
#include "fast_slow_display.h"

namespace iidxtra::fast_slow_display
{
    auto options = options_t {};

    auto update() -> void
    {
        fast_slow_hook::set_options(options);
    }

    auto reset() -> void
    {
        options = {};
        update();
    }

    // Determine if zero, fast, or slow.
    auto timing_t::get_polarity() const -> polarity
    {
        if (!std::isfinite(milliseconds) ||
            milliseconds == 0.0f) {
            return polarity::zero;
        }

        return milliseconds < 0 ? polarity::fast : polarity::slow;
    }

    // Get formatted string to display for millisecond timing.
    auto format_fastslow_ms(const float milliseconds) -> std::array<char, 7>
    {
        // +999.9 (6 characters + null terminator)
        auto result = std::array<char, 7> {};

        // Sanity check bounds.
        if (!std::isfinite(milliseconds)) {
            return result;
        }

        // Tenths of milliseconds (8.3ms --> 83).
        const auto magnitude = std::min(std::abs(milliseconds), 999.9f);
        const auto tenths = static_cast<std::uint32_t>(std::round(magnitude * 10.0f));

        // Frame perfect timing - nothing to show.
        if (tenths == 0) {
            return result;
        }

        // Format text.
        fmt::format_to_n(
            result.data(),
            result.size() - 1,
            "{}.{}",
            tenths / 10,
            tenths % 10);

        return result;
    }

    // Reset player state.
    auto display_cache_t::reset(const int player, const std::uintptr_t owner) -> void
    {
        if (player >= 0 && player < 2) {
            players_[player] = player_display_t { owner, {}, {}, {} };
        }
    }

    auto display_cache_t::update(const int player, const std::uintptr_t owner, const int code,
                                 const bool scratch, const timing_t timing) -> void
    {
        // Sanity check bounds.
        if (player != 0 && player != 1) {
            return;
        }

        if (players_[player].owner != owner) {
            reset(player, owner);
        }

        // hold for charge note
        if (code == bm2dx::judge_display_code::charge_hold) {
            return;
        }

        auto& display = players_[player];

        // Update combined f/s display.
        display.combined = timing;

        // Update separated f/s display.
        if (scratch) {
            display.scratch = display.combined;
        } else {
            display.keys = display.combined;
        }
    }

    // Get the current timing for the given player and display mode.
    auto display_cache_t::get(const int player, const std::uintptr_t owner, const bool separate,
                              const bool scratch) const -> timing_t
    {
        // Sanity check bounds.
        if (player != 0 && player != 1 ||
            players_[player].owner != owner)
            return {};

        auto const& display = players_[player];

        // Combined f/s display.
        if (!separate) {
            return display.combined;
        }

        // Separated f/s display.
        return scratch ? display.scratch : display.keys;
    }
}