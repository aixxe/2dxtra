#pragma once

#include "../game.h"

namespace iidxtra::cn_transformer
{
    extern bool enabled_p1;
    extern bool enabled_p2;

    extern int long_note_percentage_p1;
    extern int long_note_percentage_p2;

    extern int max_cn_duration_p1;
    extern int max_cn_duration_p2;

    extern bool allow_backspin_scratch_p1;
    extern bool allow_backspin_scratch_p2;

    auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;
}