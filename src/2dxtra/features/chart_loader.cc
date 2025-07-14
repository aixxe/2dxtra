#include <fstream>
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

		// Try to get an override chart from the currently active chart set.
		auto const entry_id = bm2dx::state->active_music->id;
		auto const& active_set = chart_set::custom.at(chart_set::active);

		if (!active_set.music.contains(entry_id))
			return false;

		auto& active_music = active_set.music.at(entry_id);

		if (!active_music.charts.contains(real_index))
			return false;

		// Okay, we should have an override chart by now. We still need to load it though.
		auto const& active_chart = active_music.charts.at(real_index);
		auto file = std::ifstream { active_chart.path, std::ios::binary | std::ios::ate };

		try
		{
			if (!file.good())
				return false;

			auto const size = file.tellg();

			file.seekg(0, std::ios::beg);
			file.read(static_cast<char*>(output), size);

			// Save the chart ID for later use in stage_result_hook.
			(chart_load_hook::next_player_id == 0 ?
				chart_load_hook::last_chart_id_p1:
				chart_load_hook::last_chart_id_p2) = active_chart.id;

			auto& default_set = chart_set::stock;

			auto const original_notes = default_set.music[entry_id].charts[real_index].notes;
			auto const replacement_notes = active_chart.notes;

			// If the note count changed, display the difference à la 2dxplus.
            if (replacement_notes == original_notes)
            	return true;

		    log::print("[{}] P{}: {} + {} -> {} notes", chart_set::active, chart_load_hook::next_player_id + 1,
                       original_notes, replacement_notes - original_notes, replacement_notes);
		} catch (...) {}

		return true;
	}
}