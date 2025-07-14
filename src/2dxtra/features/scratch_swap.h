#pragma once

#include "../game.h"

namespace iidxtra::scratch_swap
{
	extern bool enabled_p1;
	extern bool enabled_p2;

	extern std::int8_t swap_lane_p1;
	extern std::int8_t swap_lane_p2;

    auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
}