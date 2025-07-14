#include <meta.h>
#include "api.h"
#include "chart_set.h"
#include "score_set.h"
#include "hooks/chart_load_hook.h"

auto Version() -> const char*
	{ return VERSION_STRING; }

auto Score_Set(const char* id, std::uint8_t player, std::int32_t clear, std::int32_t miss, std::int32_t score) -> void
{
	iidxtra::score_set::custom[id].clear[player] = clear;
	iidxtra::score_set::custom[id].miss[player] = miss;
	iidxtra::score_set::custom[id].score[player] = score;
}

auto Score_Clear() -> void
	{ iidxtra::score_set::custom.clear(); }

auto Score_SetRival(const char* id, std::uint8_t rival, std::int32_t clear, std::int32_t miss, std::int32_t score) -> void
{
	if (rival >= bm2dx::MAX_RIVALS)
		return;

	iidxtra::score_set::custom_rivals[rival][id].clear = clear;
	iidxtra::score_set::custom_rivals[rival][id].miss = miss;
	iidxtra::score_set::custom_rivals[rival][id].score = score;
}

auto Score_ClearRival() -> void
{
	for (auto& set: iidxtra::score_set::custom_rivals)
		set.clear();
}

auto Chart_Exists(const char* id) -> bool
{
	// LMAO
	for (auto const& set: iidxtra::chart_set::custom)
		for (auto const& music: set.second.music)
			for (auto const& chart: music.second.charts)
				if (chart.second.id == id)
					return true;

	return false;
}

auto Is_Custom_Chart(std::uint8_t player) -> bool
{
	if (player == 0)
		return !iidxtra::chart_load_hook::last_chart_id_p1.empty();

	if (player == 1)
		return !iidxtra::chart_load_hook::last_chart_id_p2.empty();

	return false;
}

auto Get_Last_Chart_ID(std::uint8_t player) -> const char*
{
    if (player == 0)
        return iidxtra::chart_load_hook::last_chart_id_p1.c_str();

    if (player == 1)
        return iidxtra::chart_load_hook::last_chart_id_p2.c_str();

    return nullptr;
}

auto Get_Last_Note_Count(std::uint8_t player) -> std::uint32_t
{
    if (player == 0)
        return iidxtra::chart_load_hook::last_chart_note_count_p1;

    if (player == 1)
        return iidxtra::chart_load_hook::last_chart_note_count_p2;

    return 0;
}