#include "chart_set.h"
#include "score_set.h"

namespace iidxtra::chart_set
{
    auto switch_enabled = false;

	auto stock = chart_set_t {};
	auto custom = std::unordered_map<std::string, chart_set_t> {};

	auto active = std::string {};

	auto update_music_data_entry(bm2dx::music_entry_t* entry, const std::uint8_t chart_id, const chart_t& chart) -> void
	{
		if (chart_id == 3)       { entry->spb_note_count = chart.notes; entry->spb_notes_radar = chart.radar; }
		else if (chart_id == 1)  { entry->spn_note_count = chart.notes; entry->spn_notes_radar = chart.radar; }
		else if (chart_id == 0)  { entry->sph_note_count = chart.notes; entry->sph_notes_radar = chart.radar; }
		else if (chart_id == 2)  { entry->spa_note_count = chart.notes; entry->spa_notes_radar = chart.radar; }
		else if (chart_id == 4)  { entry->spl_note_count = chart.notes; entry->spl_notes_radar = chart.radar; }
		else if (chart_id == 7)  { entry->dpn_note_count = chart.notes; entry->dpn_notes_radar = chart.radar; }
		else if (chart_id == 6)  { entry->dph_note_count = chart.notes; entry->dph_notes_radar = chart.radar; }
		else if (chart_id == 8)  { entry->dpa_note_count = chart.notes; entry->dpa_notes_radar = chart.radar; }
		else if (chart_id == 10) { entry->dpl_note_count = chart.notes; entry->dpl_notes_radar = chart.radar; }
	}

	auto revert() -> void
	{
		active.clear();

		// Revert everything to default note counts
		for (auto const& [entry_id, music]: stock.music)
			for (auto const& [chart_id, original_chart]: music.charts)
				update_music_data_entry(bm2dx::music_map[entry_id], chart_id, original_chart);

		// Reset all scores
		score_set::reload_all();
	}

	auto set_active(const std::string& name) -> void
	{
		// Ensure the new set exists and isn't the currently active one.
		if (!custom.contains(name) || active == name)
			return;

		active = name;

		auto const& active_set = custom[name];

		for (auto const& [entry_id, music]: stock.music)
		{
			if (!active_set.music.contains(entry_id))
				continue;

			// Update note counts in music_data.
			for (auto const& [chart_id, original_chart]: music.charts)
			{
				if (active_set.music.at(entry_id).charts.contains(chart_id))
					update_music_data_entry(bm2dx::music_map[entry_id], chart_id, active_set.music.at(entry_id).charts.at(chart_id));
				else
					update_music_data_entry(bm2dx::music_map[entry_id], chart_id, original_chart);
			}
		}

		// Update scores.
		score_set::reload_all();
	}
}