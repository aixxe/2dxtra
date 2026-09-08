#pragma once

#include "../game.h"

namespace iidxtra::chart_speed
{
	constexpr float rate_min = 0.50f;
	constexpr float rate_max = 3.00f;

	// Current chart speed multiplier; 1.00 = stock behavior
	extern float rate;

	// Previously used multiplier by the user; used for the UI
	extern float rate_previous;

	auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
	auto install_hook() -> void;
}
