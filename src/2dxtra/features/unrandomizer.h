#pragma once

#include <array>
#include "../game.h"

namespace iidxtra::unrandomizer
{
	using random_lut_t = std::array<std::uint8_t, 7>;

	extern bool enabled_p1;
	extern bool enabled_p2;

	extern bool allow_score_save_p1;
	extern bool allow_score_save_p2;

	extern bool show_random_info_p1;
	extern bool show_random_info_p2;

	extern random_lut_t column_lut_p1;
	extern random_lut_t column_lut_p2;

	auto is_valid(std::uint8_t player) -> bool;

    auto reset() -> void;
	auto mutate(std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void;

	auto install_hook() -> void;
}