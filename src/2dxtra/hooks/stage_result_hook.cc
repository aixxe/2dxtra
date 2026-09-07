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

	// Copies the difficulty the player just finished out of the game's table
	// and into the custom chart's score slot.
	auto copy_result(score_set::score_t& target, const int player, const bm2dx::play_style style,
	                 const std::uint32_t music_id, const int difficulty) -> void
	{
		auto const* const game_score = bm2dx::player_score(player, style, music_id);

		if (game_score == nullptr || difficulty < 0 || difficulty >= 5)
			return;

		target.clear[player] = game_score->clear[difficulty];
		target.miss[player] = game_score->miss[difficulty];
		target.score[player] = game_score->score[difficulty];
	}

	auto stage_result_hook_fn(void* a1) -> std::uint8_t
	{
		auto result = reinterpret_cast<std::uint8_t (*) (void*)>(original_stage_result_hook)(a1);

		// The function we're hooking here is called AFTER the game has updated bm2dx::scores.
		// Backup the entire score structure for scores set on default charts.
		if (bm2dx::state->p1_active && chart_load_hook::last_chart_id_p1.empty())
            CopyMemory(score_set::stock_p1, bm2dx::scores[0], bm2dx::player_scores_size());

        if (bm2dx::state->p2_active && chart_load_hook::last_chart_id_p2.empty())
            CopyMemory(score_set::stock_p2, bm2dx::scores[1], bm2dx::player_scores_size());

		if (bm2dx::state->active_music == nullptr)
			return result;

		auto const music_id = static_cast<std::uint32_t>(bm2dx::state->active_music->id);

		auto score_p1 = get_custom_chart_score(chart_load_hook::last_chart_id_p1);
		auto score_p2 = get_custom_chart_score(chart_load_hook::last_chart_id_p2);

		if (bm2dx::state->play_style == 0)
		{
			if (bm2dx::state->p1_active && score_p1)
				copy_result(*score_p1, 0, bm2dx::play_style::SP, music_id, bm2dx::state->p1_difficulty);

			if (bm2dx::state->p2_active && score_p2)
				copy_result(*score_p2, 1, bm2dx::play_style::SP, music_id, bm2dx::state->p2_difficulty);
		}
		else
		{
			auto const player_index = (bm2dx::state->p1_active ? 0: 1);
			auto const difficulty_index = (bm2dx::state->p1_active
				? bm2dx::state->p1_difficulty: bm2dx::state->p2_difficulty);

			if (auto* const custom_score = (player_index == 0 ? score_p1: score_p2))
				copy_result(*custom_score, player_index, bm2dx::play_style::DP, music_id, difficulty_index);
		}

		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->STAGE_RESULT_FN, reinterpret_cast<LPVOID>(stage_result_hook_fn), &original_stage_result_hook); }
}