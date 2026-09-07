#include "../log.h"
#include "../game.h"
#include "../chart_set.h"
#include "../hooks/chart_load_hook.h"
#include "chart_loader.h"

namespace iidxtra::chart_loader
{
	auto load_custom_chart(void* output, const int index) -> bool
	{
		// Map the in-game chart index to the .1 index. (e.g. 3 -> 2 for ANOTHER)
		auto const real_index = reinterpret_cast<std::int64_t (*) (void*, int)>
			(bm2dx::addr->REMAP_INDEX_FN) (output, index);

		// Pull the mutated chart from the database; the mutated hash doubles
		// as the chart id.
		auto const pulled = chart_set::pull_chart(
			bm2dx::state->active_music->id, static_cast<int>(real_index),
			static_cast<std::uint8_t*>(output), bm2dx::CHART_BUFFER_BYTES);

		if (!pulled.has_value())
			return false;

		// Save the chart ID for later use in stage_result_hook.
		(chart_load_hook::next_player_id == 0 ?
			chart_load_hook::last_chart_id_p1:
			chart_load_hook::last_chart_id_p2) = pulled->hash;

		auto const original_notes = chart_set::stock.music[bm2dx::state->active_music->id].charts[real_index].notes;
		auto const replacement_notes = pulled->notes;

		// If the note count changed, display the difference à la 2dxplus.
        if (replacement_notes == original_notes)
        	return true;

	    log::print("[{}] P{}: {} + {} -> {} notes", chart_set::active, chart_load_hook::next_player_id + 1,
                   original_notes, replacement_notes - original_notes, replacement_notes);

		return true;
	}
}
