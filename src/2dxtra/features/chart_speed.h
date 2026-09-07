#pragma once

#include "../game.h"

namespace iidxtra::chart_speed
{
	// chart speed multiplier; 1.00 = stock behavior
	extern float rate;

	auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
	auto install_hook() -> void;
}
