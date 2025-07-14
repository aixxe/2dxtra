#pragma once

#include "game.h"
#include <cstdint>
#include <unordered_map>

namespace iidxtra::score_set
{
	struct score_t
	{
		std::int32_t clear[2];
		std::int32_t miss[2];
		std::int32_t score[2];
	};

	struct rival_score_t
	{
		std::int32_t clear;
		std::int32_t miss;
		std::int32_t score;
	};

	extern std::unordered_map<std::string, score_t> custom;
	extern std::unordered_map<std::string, rival_score_t> custom_rivals[6];

	extern bm2dx::player_scores_t* stock_p1;
	extern bm2dx::player_scores_t* stock_p2;
	extern bm2dx::rival_scores_t* stock_rivals_p1;
	extern bm2dx::rival_scores_t* stock_rivals_p2;

	auto init() -> void;
	auto uninit() -> void;
	auto backup() -> void;
	auto reload_all() -> void;
}