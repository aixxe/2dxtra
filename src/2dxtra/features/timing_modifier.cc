#include "../log.h"
#include "../hooks/score_invalidator_hook.h"
#include "timing_modifier.h"

namespace iidxtra::timing_modifier
{
    auto enabled_keys = std::array { false, false };
    auto enabled_scratch = std::array { false, false };
    auto copy_from_chart = std::array { true, true };
    auto override = std::array<bm2dx::player_timing_t, 2> {};

    auto reset() -> void
    {
        enabled_keys = { false, false };
        enabled_scratch = { false, false };
        copy_from_chart = { true, true };
        override = {};
    }

    auto is_cheating(const int player) -> bool
        { return enabled_keys[player] || enabled_scratch[player]; }

    auto execute(bm2dx::timing_data_t* data, const int player) -> void
    {
        // Copy original values into active configuration
        if (copy_from_chart[player])
        {
            override[player] = data->timing[player];
            override[player].keys.convert(false);
            override[player].scratch.convert(false);
        }

        // Check if windows are going to be modified
        if (!enabled_keys[player] && !enabled_scratch[player])
            return;

        // Invalidate this score
        score_invalidator_hook::invalidate(player);

        // Set custom values
        if (enabled_keys[player])
        {
            data->timing[player].keys = override[player].keys;
            data->timing[player].keys.convert(true);
        }

        if (enabled_scratch[player])
        {
            data->timing[player].scratch = override[player].scratch;
            data->timing[player].scratch.convert(true);
        }
    }
}