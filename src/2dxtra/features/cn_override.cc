#include "../log.h"
#include "../game.h"
#include "../hooks/score_invalidator_hook.h"
#include "cn_override.h"

namespace iidxtra::cn_override
{
    // when the chart is loaded, store the original HCN states
    auto p1_original_type = false;
    auto p2_original_type = false;

	// acceptable values: 0 = OFF, 1 = CN, 2 = HCN
	auto p1_override_type = 0;
	auto p2_override_type = 0;

    auto reset() -> void
    {
        p1_override_type = 0;
        p2_override_type = 0;
    }

    auto set_initial_states(const std::uint8_t player) -> void
    {
        auto const first_cn = std::ranges::find_if(bm2dx::play_field->notes[player],
            [] (auto const& note) { return note.type == 0 && note.end_offset != 0; });

        if (first_cn == bm2dx::play_field->notes[player].end())
            return log::debug("Set P{} original CN type to NONE", player + 1);

        (player == 0 ? p1_original_type: p2_original_type) = first_cn->hcn;
        return log::debug("Set P{} original CN type to {}", player + 1, first_cn->hcn ? "HCN": "CN");
    }

	auto update(const std::uint8_t player) -> void
    {
        if (!bm2dx::play_field || (player == 0 ? p1_override_type: p2_override_type) == 0)
            return;

        // player-specific data
        auto const old_state = player == 0 ? p1_original_type: p2_original_type;
        auto const new_state = (player == 0 ? p1_override_type: p2_override_type) == 2;

        auto modified = false;

        for (auto& note: bm2dx::play_field->notes[player])
        {
            if (note.type != 0 || note.end_offset == 0)
                continue; // ignore non-charge notes

            note.hcn = new_state;
            modified = true;
        }

        // Do nothing if the chart has no CNs.
        if (!modified)
            return;

        log::debug("Set P{} override CN type to {}", player + 1, new_state ? "HCN": "CN");

        if (old_state != new_state)
        {
            if (bm2dx::state->play_style == 1)
            {
                score_invalidator_hook::invalidate(0);
                score_invalidator_hook::invalidate(1);
            }
            else
            {
                score_invalidator_hook::invalidate(player);
            }
        }
	}
}