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

	auto select_chart_fn(void*, std::uint32_t chart[2], int) -> std::uint32_t*
	{
		auto rng = std::default_random_engine { std::random_device {} () };
		auto dist = std::uniform_int_distribution<std::size_t> { 0, bm2dx::music_map.size() - 1 };

		auto entry = bm2dx::music_map.begin();
		std::advance(entry, dist(rng));

		chart[0] = entry->first;
		chart[1] = get_highest_difficulty(entry->second);

		return chart;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->ATTRACT_SELECT_FN, select_chart_fn, &original_select_chart_fn); }
}
