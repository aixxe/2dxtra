#include "chart_set.h"
#include "score_set.h"

namespace iidxtra::chart_set
{
    auto switch_enabled = false;

	auto stock = chart_set_t {};
	auto custom = std::unordered_map<std::string, chart_set_t> {};

	auto active = std::string {};

	namespace
	{
		database::db* g_db = nullptr;

		auto inline gate_to_loader(int gate) -> std::optional<int>
		{
			static const int kMap[10] = { 3, 1, 0, 2, 4, -1, 7, 6, 8, 10 };
			return gate >= 0 && gate < 10 && kMap[gate] >= 0
				? std::optional<int>(kMap[gate]) : std::nullopt;
		}

		auto inline loader_to_gate(int loader) -> std::optional<int>
		{
			static const int kMap[11] = { 2, 1, 3, 0, 4, -1, 7, 6, 8, -1, 9 };
			return loader >= 0 && loader < 11 && kMap[loader] >= 0
				? std::optional<int>(kMap[loader]) : std::nullopt;
		}
	}

	auto update_music_data_entry(bm2dx::music_entry_t* entry, const std::uint8_t chart_id, const chart_t& chart) -> void
	{
		if (entry == nullptr)
			return;

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

	auto init(database::db* cache) -> void
		{ g_db = cache; }

	auto cache() -> database::db*
		{ return g_db; }

	auto load_sets() -> void
	{
		custom.clear();

		if (!g_db)
			return;

		for (auto const& [set_id, name]: database::list_sets(g_db))
		{
			auto result = chart_set_t {};
			result.id = set_id;

			auto charts_added = 0;

			for (auto const& row: database::charts_for_set(g_db, set_id))
			{
				auto const loader_index = gate_to_loader(row.difficulty);

				if (!loader_index.has_value())
					continue;

				auto& music_entry = result.music[row.music_id];

				music_entry.charts[loader_index.value()] = chart_t {
					.id    = row.hash,
					.notes = row.notes,
					.radar = bm2dx::notes_radar_t {
						row.radar[0], row.radar[1], row.radar[2],
						row.radar[3], row.radar[4], row.radar[5],
					},
				};

				charts_added++;
			}

			result.count = charts_added;
			custom[name] = std::move(result);
		}
	}

	auto exists(const std::string& id) -> bool
		{ return database::exists(g_db, id); }

	auto pull_chart(int music_id, int loader_index, std::uint8_t* dst,
	                std::size_t capacity) -> std::optional<database::pulled_chart>
	{
		if (active.empty() || !g_db || !custom.contains(active))
			return std::nullopt;

		auto const gate = loader_to_gate(loader_index);
		if (!gate.has_value())
			return std::nullopt;

		return database::pull(g_db, custom.at(active).id, music_id,
			gate.value(), dst, capacity);
	}

	auto revert() -> void
	{
		active.clear();

		for (auto const& [entry_id, music]: stock.music)
		{
			auto const entry = bm2dx::music_map.find(entry_id);

			if (entry == bm2dx::music_map.end())
				continue;

			for (auto const& [chart_id, original_chart]: music.charts)
				update_music_data_entry(entry->second, chart_id, original_chart);
		}

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
			auto const custom_music = active_set.music.find(entry_id);
			auto const entry = bm2dx::music_map.find(entry_id);

			if (custom_music == active_set.music.end() || entry == bm2dx::music_map.end())
				continue;

			// Update note counts in music_data.
			for (auto const& [chart_id, original_chart]: music.charts)
			{
				auto const custom_chart = custom_music->second.charts.find(chart_id);

				update_music_data_entry(entry->second, chart_id,
					custom_chart != custom_music->second.charts.end()
						? custom_chart->second: original_chart);
			}
		}

		// Update scores.
		score_set::reload_all();
	}
}