#pragma once

#include <array>
#include "../game.h"

namespace iidxtra::timing_modifier
{
	extern std::array<bool, 2> enabled_keys;
	extern std::array<bool, 2> enabled_scratch;
	extern std::array<bool, 2> copy_from_chart;
    extern std::array<bm2dx::player_timing_t, 2> override;

	auto reset() -> void;

    auto is_cheating(int player) -> bool;
    auto execute(bm2dx::timing_data_t* data, int player) -> void;
}