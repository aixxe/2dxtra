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

	bm2dx::game_score_t* stock_p1 = nullptr;
	bm2dx::game_score_t* stock_p2 = nullptr;
	bm2dx::game_score_t* stock_rivals_p1 = nullptr;
	bm2dx::game_score_t* stock_rivals_p2 = nullptr;

	auto init() -> bool
	{
		auto const alloc = [] (std::size_t size)
			{ return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size); };

		auto const player_size = bm2dx::player_scores_size();
		auto const rival_size = bm2dx::rival_scores_size();

		if (player_size == 0 || rival_size == 0)
			return false;

		stock_p1 = static_cast<bm2dx::game_score_t*>(alloc(player_size));
		stock_p2 = static_cast<bm2dx::game_score_t*>(alloc(player_size));
		stock_rivals_p1 = static_cast<bm2dx::game_score_t*>(alloc(rival_size));
		stock_rivals_p2 = static_cast<bm2dx::game_score_t*>(alloc(rival_size));

		return stock_p1 && stock_p2 && stock_rivals_p1 && stock_rivals_p2;
	}

	void uninit()
	{
		for (auto* block: { static_cast<void*>(stock_p1), static_cast<void*>(stock_p2),
		                    static_cast<void*>(stock_rivals_p1), static_cast<void*>(stock_rivals_p2) })
		{
			if (block != nullptr)
				HeapFree(GetProcessHeap(), 0, block);
		}

		stock_p1 = nullptr;
		stock_p2 = nullptr;
		stock_rivals_p1 = nullptr;
		stock_rivals_p2 = nullptr;
	}

	void backup()
	{
		if (stock_p1 == nullptr)
			return;

		log::debug("Copying scores for default charts...");

		if (bm2dx::state->p1_active)
		{
			CopyMemory(stock_p1, bm2dx::scores[0], bm2dx::player_scores_size());
			CopyMemory(stock_rivals_p1, bm2dx::rival_scores[0], bm2dx::rival_scores_size());
		}

		if (bm2dx::state->p2_active)
		{
			CopyMemory(stock_p2, bm2dx::scores[1], bm2dx::player_scores_size());
			CopyMemory(stock_rivals_p2, bm2dx::rival_scores[1], bm2dx::rival_scores_size());
		}
	}

	auto clear_stock() -> void
	{
		if (stock_p1 == nullptr)
			return;

		ZeroMemory(stock_p1, bm2dx::player_scores_size());
		ZeroMemory(stock_p2, bm2dx::player_scores_size());
	}

	namespace
	{
		auto apply_score(bm2dx::game_score_t* target, const std::uint32_t score_index,
		                 const std::int32_t clear, const std::int32_t miss,
		                 const std::int32_t score) -> void
		{
			if (target == nullptr)
				return;

			target->clear[score_index] = static_cast<std::int8_t>(clear);
			target->miss[score_index] = miss;
			target->score[score_index] = score;
			target->is_populated = 1;
		}
	}

	void reload_all()
	{
		if (stock_p1 == nullptr)
			return;

		// Copy default scores back in if there's no custom chart set active.
		if (chart_set::active.empty())
		{
			// Simply restore from the backed up network score sets.
			if (bm2dx::state->p1_active)
			{
				CopyMemory(bm2dx::scores[0], stock_p1, bm2dx::player_scores_size());
				CopyMemory(bm2dx::rival_scores[0], stock_rivals_p1, bm2dx::rival_scores_size());
			}

			if (bm2dx::state->p2_active)
			{
				CopyMemory(bm2dx::scores[1], stock_p2, bm2dx::player_scores_size());
				CopyMemory(bm2dx::rival_scores[1], stock_rivals_p2, bm2dx::rival_scores_size());
			}

			return;
		}

		// Clear all stock scores in advance, rather than X times.
		ZeroMemory(bm2dx::scores[0], bm2dx::player_scores_size());
		ZeroMemory(bm2dx::scores[1], bm2dx::player_scores_size());
		ZeroMemory(bm2dx::rival_scores[0], bm2dx::rival_scores_size());
		ZeroMemory(bm2dx::rival_scores[1], bm2dx::rival_scores_size());

		auto& set = chart_set::custom.at(chart_set::active);
		auto const active_player = bm2dx::state->p1_active ? 0: 1;

		// Loop through the stock music set since it'll have all the IDs.
		for (auto const& [entry_id, music]: chart_set::stock.music)
		{
			// Cleaning out the scores is enough if this isn't a custom score set.
			// Otherwise, does the custom chart set have anything for this song ID?
			auto const custom_music = set.music.find(entry_id);

			if (custom_music == set.music.end())
				continue;

			// Okay, let's loop through the charts.
			for (auto const& [chart_index, chart]: custom_music->second.charts)
			{
				auto const sp = mapping_sp.find(chart_index);
				auto const dp = mapping_dp.find(chart_index);

				if (sp == mapping_sp.end() && dp == mapping_dp.end())
					continue;

				auto const style = sp != mapping_sp.end()
					? bm2dx::play_style::SP: bm2dx::play_style::DP;
				auto const score_index = sp != mapping_sp.end() ? sp->second: dp->second;

				// check all rivals to see if they have a score on this custom chart
				for (auto rival_idx = 0; rival_idx < bm2dx::MAX_RIVALS; ++rival_idx)
				{
					auto const rival = custom_rivals[rival_idx].find(chart.id);

					if (rival == custom_rivals[rival_idx].end())
						continue;

					apply_score(bm2dx::rival_score(active_player, rival_idx, style, entry_id),
						score_index, rival->second.clear, rival->second.miss, rival->second.score);
				}

				auto const own = custom.find(chart.id);

				if (own == custom.end())
					continue;

				if (style == bm2dx::play_style::SP)
				{
					for (auto player = 0; player < 2; ++player)
					{
						apply_score(bm2dx::player_score(player, style, entry_id), score_index,
							own->second.clear[player], own->second.miss[player], own->second.score[player]);
					}
				}
				else
				{
					apply_score(bm2dx::player_score(active_player, style, entry_id), score_index,
						own->second.clear[active_player], own->second.miss[active_player],
						own->second.score[active_player]);
				}
			}
		}
	}
}