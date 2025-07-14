#include <mutex>
#include <meta.h>
#include <MinHook.h>
#include <rapidxml_utils.hpp>
#include "../log.h"
#include "../game.h"
#include "../chart_set.h"
#include "mdata_load_hook.h"
#include "renderer_hook.h"

namespace iidxtra::mdata_load_hook
{
	void* original_mdata_load_fn = nullptr;

	auto input_sets = std::vector<std::filesystem::path> {};

	auto inline remap_difficulty_index(const std::string& name) -> std::optional<std::uint8_t>
	{
		auto static indexes = std::unordered_map<std::string, std::uint8_t> {
			{"spb", 3},
			{"spn", 1},
			{"sph", 0},
			{"spa", 2},
			{"spl", 4},
			{"dpn", 7},
			{"dph", 6},
			{"dpa", 8},
			{"dpl", 10},
		};

		if (!indexes.contains(name))
			return std::nullopt;

		return indexes[name];
	}

	auto populate_default_set() -> void
	{
		auto const music_data = reinterpret_cast<bm2dx::music_data_t* (*) ()>
			(bm2dx::addr->GET_MUSIC_DATA) ();

		// Reserve memory for the amount of occupied entries in advance.
		chart_set::stock.music.reserve(music_data->occupied_entries);

		// Loop through entries until we hit the end.
		for (auto entry = &music_data->first; entry->id != 0; entry++)
		{
			auto& music_entry = chart_set::stock.music[entry->id];

			// Add all the charts using the loader ID as the key.
			if (entry->spb_rating != 0) music_entry.charts[3]  = {"", "", entry->spb_note_count, entry->spb_notes_radar};
			if (entry->spn_rating != 0) music_entry.charts[1]  = {"", "", entry->spn_note_count, entry->spn_notes_radar};
			if (entry->sph_rating != 0) music_entry.charts[0]  = {"", "", entry->sph_note_count, entry->sph_notes_radar};
			if (entry->spa_rating != 0) music_entry.charts[2]  = {"", "", entry->spa_note_count, entry->spa_notes_radar};
			if (entry->spl_rating != 0) music_entry.charts[4]  = {"", "", entry->spl_note_count, entry->spl_notes_radar};
			if (entry->dpn_rating != 0) music_entry.charts[7]  = {"", "", entry->dpn_note_count, entry->dpn_notes_radar};
			if (entry->dph_rating != 0) music_entry.charts[6]  = {"", "", entry->dph_note_count, entry->dph_notes_radar};
			if (entry->dpa_rating != 0) music_entry.charts[8]  = {"", "", entry->dpa_note_count, entry->dpa_notes_radar};
			if (entry->dpl_rating != 0) music_entry.charts[10] = {"", "", entry->dpl_note_count, entry->dpl_notes_radar};

			// Populate hash map for future use.
			bm2dx::music_map[entry->id] = entry;
		}
	}

	auto gather_custom_sets(const std::filesystem::path& path) -> void
	{
		try
		{
			{
				auto result = std::vector<std::filesystem::path> {};
				auto directory = std::filesystem::directory_iterator(path);

				for (auto& file: directory)
				{
					if (file.is_directory())
						continue;

					auto filename = file.path().string();

					if (!filename.ends_with(".xml"))
						continue;

					input_sets.emplace_back(file);
				}
			}
		} catch (...) { }
	}

	auto populate_custom_sets() -> void
	{
		try
		{
			for (auto const& input_filename: input_sets)
			{
				// Error and validity checking go here.
				auto file = std::ifstream { input_filename };
				auto xml_file = rapidxml::file { file };
				auto document = rapidxml::xml_document {};

				document.parse<0>(xml_file.data());

				// Get the base directory since chart paths are relative to it.
				auto basedir = input_filename;
				basedir.remove_filename();

				// Get filename and strip extension for in-game display name.
				auto basename = input_filename.filename().replace_extension().string();

				// Root is an object keyed by music entry IDs.
				auto result = chart_set::chart_set_t {};
				auto root_node = document.first_node("entries");

				// We have to loop through to get the count.
				// This is probably more efficient than letting it constantly reallocate itself.
				{
					auto count = 0;
					for (auto music = root_node->first_node("music"); music; music = music->next_sibling())
						count++;
					result.music.reserve(count);
				}

				auto charts_added = 0;

				for (auto music = root_node->first_node("music"); music; music = music->next_sibling())
				{
					auto entry_id = std::stoi(music->first_attribute("id")->value());

					// Make sure this is a known entry in the default set.
					if (!chart_set::stock.music.contains(entry_id))
						continue;

					// Populate a music entry for this set.
					auto& music_entry = result.music[entry_id];

					for (auto chart = music->first_node("chart"); chart; chart = chart->next_sibling())
					{
						auto difficulty = chart->first_attribute("type")->value();
						auto index = remap_difficulty_index(difficulty);

						if (!index.has_value())
							continue;

						// Make sure this is a known chart in the default set.
						if (!chart_set::stock.music.at(entry_id).charts.contains(index.value()))
							continue;

						// Populate a chart for this set.
						auto id = chart->first_attribute("id")->value();
						auto path = basedir;
						path.append(chart->first_attribute("path")->value());

						// Radar data
						auto radar = bm2dx::notes_radar_t {};

						if (auto radar_node = chart->first_node("radar"))
						{
							if (radar_node->first_attribute("notes"))
								radar.notes = static_cast<int>(std::stoul(radar_node->first_attribute("notes")->value()));

							if (radar_node->first_attribute("peak"))
								radar.peak = static_cast<int>(std::stoul(radar_node->first_attribute("peak")->value()));

							if (radar_node->first_attribute("scratch"))
								radar.scratch = static_cast<int>(std::stoul(radar_node->first_attribute("scratch")->value()));

							if (radar_node->first_attribute("soflan"))
								radar.soflan = static_cast<int>(std::stoul(radar_node->first_attribute("soflan")->value()));

							if (radar_node->first_attribute("charge"))
								radar.charge = static_cast<int>(std::stoul(radar_node->first_attribute("charge")->value()));

							if (radar_node->first_attribute("chord"))
								radar.chord = static_cast<int>(std::stoul(radar_node->first_attribute("chord")->value()));
						}

						charts_added++;

						music_entry.charts[index.value()] = chart_set::chart_t {
							.id    = id,
							.path  = path,
							.notes = static_cast<std::uint32_t>(std::stoul(chart->first_attribute("notes")->value())),
							.radar = radar
						};
					}
				}

				result.count = charts_added;
				chart_set::custom[basename] = std::move(result);
			}
		} catch (...) {}
	}

	auto init_music_database() -> void
	{
		auto static run_once = std::once_flag {};

		std::call_once(run_once, []
		{
		    // Step 0: Initialize the renderer.
            renderer_hook::install_hook();

			// Step 1: Populate the default chart set.
			populate_default_set();

			// Step 2: Populate custom chart sets.
			populate_custom_sets();

			// Step 3: Update logger framerate.
			log::base_framerate = bm2dx::config->target_fps;

			// Step 4: Print some post-init text.
			log::init("Welcome to 2dxtra! (v{} - by aixxe)", VERSION_STRING);
			log::init("Press EFFECT twice to open the options menu");
		});
	}

	auto mdata_load_hook_fn(void* a1) -> void
	{
		init_music_database();

		reinterpret_cast<void (*) (void*)>(original_mdata_load_fn)(a1);
	}

	auto install_hook() -> void
	{
	#ifndef NDEBUG
		// assume music_data is already loaded
		init_music_database();
	#else
		// hook some random function that gets called shortly after the music_data structure is fully populated
		MH_CreateHook(bm2dx::addr->MDATA_LOAD_FN, mdata_load_hook_fn, &original_mdata_load_fn);
	#endif
	}
}