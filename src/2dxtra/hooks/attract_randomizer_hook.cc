#include <random>
#include <MinHook.h>
#include "../game.h"
#include "attract_randomizer_hook.h"

namespace iidxtra::attract_randomizer_hook
{
	void* original_select_chart_fn = nullptr;

	auto get_highest_difficulty(const bm2dx::music_entry_t* entry)
	{
		if (entry->spl_rating > 0) return 4;
		if (entry->spa_rating > 0) return 3;
		if (entry->sph_rating > 0) return 2;
		if (entry->spn_rating > 0) return 1;
		return 0;
	}

	auto select_chart_fn(void* a1, std::uint32_t chart[2], int a3) -> std::uint32_t*
	{
		// music_data has not been walked yet during early boot; let the game
		// pick in that case rather than indexing an empty map.
		if (bm2dx::music_map.empty())
		{
			return reinterpret_cast<std::uint32_t* (*) (void*, std::uint32_t*, int)>
				(original_select_chart_fn) (a1, chart, a3);
		}

		auto rng = std::default_random_engine { std::random_device {} () };
		auto dist = std::uniform_int_distribution<std::size_t> { 0, bm2dx::music_map.size() - 1 };

		auto entry = bm2dx::music_map.begin();
		std::advance(entry, dist(rng));

		chart[0] = entry->first;
		chart[1] = get_highest_difficulty(entry->second);

		return chart;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->ATTRACT_SELECT_FN, reinterpret_cast<LPVOID>(select_chart_fn), &original_select_chart_fn); }
}
