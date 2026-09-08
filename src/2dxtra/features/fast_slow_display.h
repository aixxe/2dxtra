#pragma once

#include <array>
#include <cstdint>

namespace iidxtra::fast_slow_display
{
    extern bool enabled;

    auto update() -> void;
    auto reset() -> void;

    enum class polarity { zero, fast, slow };

    struct timing_t
    {
        float milliseconds = 0.0f;

        auto get_polarity() const -> polarity;
    };

    struct player_display_t
    {
        // which player?
        std::uintptr_t owner = 0;
        timing_t combined;
        timing_t keys;
        timing_t scratch;
    };

    class display_cache_t
    {
    public:
        auto reset(int player, std::uintptr_t owner) -> void;
        auto update(int player, std::uintptr_t owner, int code, bool scratch, timing_t timing) -> void;
        auto get(int player, std::uintptr_t owner, bool separate, bool scratch) const -> timing_t;

    private:
        std::array<player_display_t, 2> players_ {};
    };

    auto format_fastslow_ms(float milliseconds) -> std::array<char, 7>;
}