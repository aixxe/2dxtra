#include <MinHook.h>
#include "../chart_set.h"
#include "../score_set.h"
#include "chart_load_hook.h"
#include "stage_result_hook.h"

namespace iidxtra::stage_result_hook
{
	void* original_stage_result_hook = nullptr;

	auto get_custom_chart_score(const std::string& chart_id) -> score_set::score_t*
	{
		if (chart_id.empty())
			return nullptr;

		if (!score_set::custom.contains(chart_id))
			score_set::custom[chart_id] = {};

		return &score_set::custom[chart_id];
	}

	auto stage_result_hook_fn(void* a1) -> std::uint8_t
	{
		auto result = reinterpret_cast<std::uint8_t (*) (void*)>(original_stage_result_hook)(a1);

		// The function we're hooking here is called AFTER the game has updated bm2dx::scores.
		// Backup the entire score structure for scores set on default charts.
		if (bm2dx::state->p1_active && chart_load_hook::last_chart_id_p1.empty())
            CopyMemory(score_set::stock_p1, bm2dx::scores[0], sizeof(bm2dx::player_scores_t));

        if (bm2dx::state->p2_active && chart_load_hook::last_chart_id_p2.empty())
            CopyMemory(score_set::stock_p2, bm2dx::scores[1], sizeof(bm2dx::player_scores_t));

		auto score_p1 = get_custom_chart_score(chart_load_hook::last_chart_id_p1);
		auto score_p2 = get_custom_chart_score(chart_load_hook::last_chart_id_p2);

		if (bm2dx::state->play_style == 0)
		{
			if (bm2dx::state->p1_active && score_p1)
			{
				auto& game_score = bm2dx::scores[0]->sp[bm2dx::state->active_music->id];

				score_p1->clear[0] = game_score.clear[bm2dx::state->p1_difficulty];
				score_p1->miss[0] = game_score.miss[bm2dx::state->p1_difficulty];
				score_p1->score[0] = game_score.score[bm2dx::state->p1_difficulty];
			}

			if (bm2dx::state->p2_active && score_p2)
			{
				auto& game_score = bm2dx::scores[1]->sp[bm2dx::state->active_music->id];

				score_p2->clear[1] = game_score.clear[bm2dx::state->p2_difficulty];
				score_p2->miss[1] = game_score.miss[bm2dx::state->p2_difficulty];
				score_p2->score[1] = game_score.score[bm2dx::state->p2_difficulty];
			}
		}
		else
		{
			auto player_index = (bm2dx::state->p1_active ? 0: 1);
			auto difficulty_index = (bm2dx::state->p1_active ? bm2dx::state->p1_difficulty: bm2dx::state->p2_difficulty);

			auto& game_score = bm2dx::scores[player_index]->dp[bm2dx::state->active_music->id];
			auto& custom_score = (player_index == 0 ? score_p1: score_p2);

			if (custom_score)
			{
				custom_score->clear[player_index] = game_score.clear[difficulty_index];
				custom_score->miss[player_index] = game_score.miss[difficulty_index];
				custom_score->score[player_index] = game_score.score[difficulty_index];
			}
		}

		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->STAGE_RESULT_FN, stage_result_hook_fn, &original_stage_result_hook); }
}