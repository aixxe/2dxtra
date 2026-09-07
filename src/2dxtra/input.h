#pragma once

#include "game.h"

namespace iidxtra::input
{
	extern bm2dx::input_t menu;

    auto test_turntable(bool cw) -> bool;
	auto test_game_button(bm2dx::button button) -> bool;
	auto test_menu_button(std::uint8_t button) -> bool;
}