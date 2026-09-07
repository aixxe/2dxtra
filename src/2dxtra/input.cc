#include <bitset>
#include "input.h"

auto constexpr turntable_debounce_frames = 10;

namespace iidxtra::input
{
	bm2dx::input_t menu = {};

    auto test_turntable(const bool cw) -> bool
    {
        auto const left = menu.p1_turntable_delta <= -1 || menu.p2_turntable_delta <= -1;
        auto const right = menu.p1_turntable_delta >= 1 || menu.p2_turntable_delta >= 1;

        // don't debounce if false
        if ((!cw && !left) || (cw && !right))
            return false;

        // input state is true now, but we need to debounce
        auto static last_frame = 0;
        last_frame++;

        if (last_frame < turntable_debounce_frames)
            return false;

        if (last_frame == turntable_debounce_frames)
            last_frame = 0;

        return true;
    }

	auto test_game_button(const bm2dx::button button) -> bool
    {
	    return std::bitset<32>(bm2dx::input_manager->data.buttons_edge)
	        .test(static_cast<std::size_t>(button));
    }

	auto test_menu_button(const std::uint8_t button) -> bool
        { return std::bitset<32>(menu.buttons).test(button); }
}