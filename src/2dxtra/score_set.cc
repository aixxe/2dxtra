#include <windows.h>
#include "log.h"
#include "chart_set.h"
#include "score_set.h"

namespace iidxtra::score_set
{
	// first = chart index as used by the loader
	// second = index used by game score structs
	auto mapping_sp = std::unordered_map<std::uint32_t, std::uint32_t> {
		{3, 0},  // SP BEGINNER
		{1, 1},  // SP NORMAL
		{0, 2},  // SP HYPER
		{2, 3},  // SP ANOTHER
		{4, 4},  // SP LEGGENDARIA
	};

	auto mapping_dp = std::unordered_map<std::uint32_t, std::uint32_t> {
		{7, 1},  // DP NORMAL
		{6, 2},  // DP HYPER
		{8, 3},  // DP ANOTHER
		{10, 4}, // DP LEGGENDARIA
	};

	std::unordered_map<std::string, score_t> custom;
	std::unordered_map<std::string, rival_score_t> custom_rivals[6];

	bm2dx::player_scores_t* stock_p1 = nullptr;
	bm2dx::player_scores_t* stock_p2 = nullptr;
	bm2dx::rival_scores_t* stock_rivals_p1 = nullptr;
	bm2dx::rival_scores_t* stock_rivals_p2 = nullptr;

	auto init() -> void
	{
		stock_p1 = static_cast<bm2dx::player_scores_t*>(HeapAlloc(GetProcessHeap(), 0, sizeof(bm2dx::player_scores_t)));
		stock_p2 = static_cast<bm2dx::player_scores_t*>(HeapAlloc(GetProcessHeap(), 0, sizeof(bm2dx::player_scores_t)));
		stock_rivals_p1 = static_cast<bm2dx::rival_scores_t*>(HeapAlloc(GetProcessHeap(), 0, sizeof(bm2dx::rival_scores_t)));
		stock_rivals_p2 = static_cast<bm2dx::rival_scores_t*>(HeapAlloc(GetProcessHeap(), 0, sizeof(bm2dx::rival_scores_t)));
	}

	void uninit()
	{
		HeapFree(GetProcessHeap(), 0, stock_p1);
		HeapFree(GetProcessHeap(), 0, stock_p2);
		HeapFree(GetProcessHeap(), 0, stock_rivals_p1);
		HeapFree(GetProcessHeap(), 0, stock_rivals_p2);
	}

	void backup()
	{
		log::debug("Copying scores for default charts...");

		if (bm2dx::state->p1_active)
		{
			CopyMemory(score_set::stock_p1, bm2dx::scores[0], sizeof(bm2dx::player_scores_t));
			CopyMemory(score_set::stock_rivals_p1, bm2dx::rival_scores[0], sizeof(bm2dx::rival_scores_t));
		}

		if (bm2dx::state->p2_active)
		{
			CopyMemory(score_set::stock_p2, bm2dx::scores[1], sizeof(bm2dx::player_scores_t));
			CopyMemory(score_set::stock_rivals_p2, bm2dx::rival_scores[1], sizeof(bm2dx::rival_scores_t));
		}
	}

	void reload_all()
	{
		// Copy default scores back in if there's no custom chart set active.
		if (chart_set::active.empty())
		{
			// Simply restore from the backed up network score sets.
			if (bm2dx::state->p1_active)
			{
				CopyMemory(bm2dx::scores[0], score_set::stock_p1, sizeof(bm2dx::player_scores_t));
				CopyMemory(bm2dx::rival_scores[0], score_set::stock_rivals_p1, sizeof(bm2dx::rival_scores_t));
			}

			if (bm2dx::state->p2_active)
			{
				CopyMemory(bm2dx::scores[1], score_set::stock_p2, sizeof(bm2dx::player_scores_t));
				CopyMemory(bm2dx::rival_scores[1], score_set::stock_rivals_p2, sizeof(bm2dx::rival_scores_t));
			}

			return;
		}

		// Clear all stock scores in advance, rather than X times.
		ZeroMemory(bm2dx::scores[0], sizeof(bm2dx::player_scores_t));
		ZeroMemory(bm2dx::scores[1], sizeof(bm2dx::player_scores_t));
		ZeroMemory(bm2dx::rival_scores[0], sizeof(bm2dx::rival_scores_t));
		ZeroMemory(bm2dx::rival_scores[1], sizeof(bm2dx::rival_scores_t));

		auto set = chart_set::custom.at(chart_set::active);

		// Loop through the stock music set since it'll have all the IDs.
		for (auto const& [entry_id, music]: chart_set::stock.music)
		{
			// Cleaning out the scores is enough if this isn't a custom score set.
			// Otherwise, does the custom chart set have anything for this song ID?
			if (!set.music.contains(entry_id))
				continue;

			// Okay, let's loop through the charts.
			for (auto const& [chart_index, chart]: set.music[entry_id].charts)
			{
				// check all rivals to see if they have a score on this custom chart
				auto const active_player = bm2dx::state->p1_active ? 0: 1;

				for (auto rival_idx = 0; rival_idx < bm2dx::MAX_RIVALS; ++rival_idx)
				{
					if (!custom_rivals[rival_idx].contains(chart.id))
						continue;

					if (mapping_sp.contains(chart_index))
					{
						auto const score_index = mapping_sp[chart_index];

						bm2dx::rival_scores[active_player]->sp[rival_idx][entry_id].clear[score_index] = custom_rivals[rival_idx][chart.id].clear;
						bm2dx::rival_scores[active_player]->sp[rival_idx][entry_id].miss[score_index] = custom_rivals[rival_idx][chart.id].miss;
						bm2dx::rival_scores[active_player]->sp[rival_idx][entry_id].score[score_index] = custom_rivals[rival_idx][chart.id].score;
						bm2dx::rival_scores[active_player]->sp[rival_idx][entry_id].is_populated = 1;
					}
					else if (mapping_dp.contains(chart_index))
					{
						auto const score_index = mapping_dp[chart_index];

						bm2dx::rival_scores[active_player]->dp[rival_idx][entry_id].clear[score_index] = custom_rivals[rival_idx][chart.id].clear;
						bm2dx::rival_scores[active_player]->dp[rival_idx][entry_id].miss[score_index] = custom_rivals[rival_idx][chart.id].miss;
						bm2dx::rival_scores[active_player]->dp[rival_idx][entry_id].score[score_index] = custom_rivals[rival_idx][chart.id].score;
						bm2dx::rival_scores[active_player]->dp[rival_idx][entry_id].is_populated = 1;
					}
				}

				if (!custom.contains(chart.id))
					continue;

				// A custom score exists for this chart.
				if (mapping_sp.contains(chart_index))
				{
					auto const score_index = mapping_sp[chart_index];

					bm2dx::scores[0]->sp[entry_id].clear[score_index] = custom[chart.id].clear[0];
					bm2dx::scores[0]->sp[entry_id].miss[score_index] = custom[chart.id].miss[0];
					bm2dx::scores[0]->sp[entry_id].score[score_index] = custom[chart.id].score[0];
					bm2dx::scores[0]->sp[entry_id].is_populated = 1;

					bm2dx::scores[1]->sp[entry_id].clear[score_index] = custom[chart.id].clear[1];
					bm2dx::scores[1]->sp[entry_id].miss[score_index] = custom[chart.id].miss[1];
					bm2dx::scores[1]->sp[entry_id].score[score_index] = custom[chart.id].score[1];
					bm2dx::scores[1]->sp[entry_id].is_populated = 1;
				}
				else if (mapping_dp.contains(chart_index))
				{
					auto const player_index = (bm2dx::state->p1_active ? 0: 1);
					auto const score_index = mapping_dp[chart_index];

					bm2dx::scores[player_index]->dp[entry_id].clear[score_index] = custom[chart.id].clear[player_index];
					bm2dx::scores[player_index]->dp[entry_id].miss[score_index] = custom[chart.id].miss[player_index];
					bm2dx::scores[player_index]->dp[entry_id].score[score_index] = custom[chart.id].score[player_index];
					bm2dx::scores[player_index]->dp[entry_id].is_populated = 1;
				}
			}
		}
	}
}