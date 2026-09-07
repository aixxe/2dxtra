#include <mutex>
#include <meta.h>
#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../chart_set.h"
#include "mdata_load_hook.h"
#include "renderer_hook.h"

namespace iidxtra::mdata_load_hook
{
	void* original_mdata_load_fn = nullptr;

	auto populate_default_set() -> void
	{
		auto const music_data = reinterpret_cast<bm2dx::music_data_t* (*) ()>
			(bm2dx::addr->GET_MUSIC_DATA) ();

		if (music_data == nullptr)
			return;

		// Reserve memory for the amount of occupied entries in advance.
		chart_set::stock.music.reserve(music_data->occupied_entries);

		// Walk the entries until the terminator, but never past the table the
		// blob is declared to hold.
		auto* const entries = bm2dx::music_first(music_data);
		auto const* const limit = entries + bm2dx::max_entries();

		for (auto entry = entries; entry < limit && entry->id != 0; entry++)
		{
			auto& music_entry = chart_set::stock.music[entry->id];

			// Add all the charts using the loader ID as the key.
			if (entry->spb_rating != 0) music_entry.charts[3]  = {"", entry->spb_note_count, entry->spb_notes_radar};
			if (entry->spn_rating != 0) music_entry.charts[1]  = {"", entry->spn_note_count, entry->spn_notes_radar};
			if (entry->sph_rating != 0) music_entry.charts[0]  = {"", entry->sph_note_count, entry->sph_notes_radar};
			if (entry->spa_rating != 0) music_entry.charts[2]  = {"", entry->spa_note_count, entry->spa_notes_radar};
			if (entry->spl_rating != 0) music_entry.charts[4]  = {"", entry->spl_note_count, entry->spl_notes_radar};
			if (entry->dpn_rating != 0) music_entry.charts[7]  = {"", entry->dpn_note_count, entry->dpn_notes_radar};
			if (entry->dph_rating != 0) music_entry.charts[6]  = {"", entry->dph_note_count, entry->dph_notes_radar};
			if (entry->dpa_rating != 0) music_entry.charts[8]  = {"", entry->dpa_note_count, entry->dpa_notes_radar};
			if (entry->dpl_rating != 0) music_entry.charts[10] = {"", entry->dpl_note_count, entry->dpl_notes_radar};

			// Populate hash map for future use.
			bm2dx::music_map[entry->id] = entry;
		}
	}

	auto init_music_database() -> void
	{
		auto static run_once = std::once_flag {};

		std::call_once(run_once, []
		{
			// Chart mutation is finished by the time this hook fires: flush
			// the journal-less writes out of the pager cache so the main
			// database file is complete on disk.
			database::flush(chart_set::cache());

		    // Step 0: Initialize the renderer.
            renderer_hook::install_hook();

			// Step 1: Populate the default chart set.
			populate_default_set();

			// Step 2: Update logger framerate.
			log::base_framerate = bm2dx::config->target_fps;

			// Step 3: Print some post-init text.
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
		MH_CreateHook(bm2dx::addr->MDATA_LOAD_FN, reinterpret_cast<LPVOID>(mdata_load_hook_fn), &original_mdata_load_fn);
	#endif
	}
}