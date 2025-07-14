#pragma once

#include "../game.h"

namespace iidxtra::regular_speed
{
	extern bool enabled;

    auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
}