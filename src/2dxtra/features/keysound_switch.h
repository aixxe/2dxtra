#pragma once

#include "../game.h"

namespace iidxtra::keysound_switch
{
    extern int override_type;
    extern bool mute_bgm;

    auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
}