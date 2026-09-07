#include <bitset>
#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../input.h"
#include "../gui/gui.h"
#include "input_hook.h"

namespace iidxtra::input_hook
{
	void* (*original_input_fn) (bm2dx::InputManagerIIDX*) = nullptr;

	// How long the second EFFECT tap may arrive after the first, in seconds.
	auto constexpr double_tap_window = 0.2;

	auto input_hook_fn(bm2dx::InputManagerIIDX* a1) -> void*
	{
		auto static timeout = 0;
		auto static old_state = bm2dx::input_t {};

		// The poll fn's input data lives in two disjoint regions of the object:
		// the button bitfields at the front and the turntable slots further in.
		// The bytes between them are the game's own list/map headers - copying
		// those back corrupts the input manager and crashes on the next poll,
		// so only the two named windows are ever touched.
		auto constexpr buttons_size = bm2dx::INPUT_BUTTON_BYTES;
		auto constexpr turntable_size = bm2dx::INPUT_TURNTABLE_BYTES;

		auto const effect_bit = static_cast<std::size_t>(bm2dx::button::EFFECT);

		// menu is visible -- lock inputs from the game
		if (gui::visible)
		{
			CopyMemory(&old_state.buttons, &a1->data.buttons, buttons_size);
			CopyMemory(&old_state.p1_turntable, &a1->data.p1_turntable, turntable_size);
		}

		// get the new inputs and feed them to the menu
		auto const result = original_input_fn(a1);

		if (gui::visible)
		{
			CopyMemory(&input::menu.buttons, &a1->data.buttons, buttons_size);
			CopyMemory(&input::menu.p1_turntable, &a1->data.p1_turntable, turntable_size);
		}

		// toggle the gui state
		{
			if (timeout > 0)
				timeout--;

			if (std::bitset<32>(a1->data.buttons).test(effect_bit))
			{
				if (timeout > 0)
				{
					// second tap occurred within the window
					// check if we're allowed to open the gui
					if (!gui::visible && gui::play_lock_state && bm2dx::play_session->in_gameplay)
						log::print("Menu is currently unavailable");
					else
						gui::visible = !gui::visible;

					timeout = 0;
				}
				else
				{
					// now waiting for the next tap
					timeout = static_cast<int>(bm2dx::config->target_fps * double_tap_window);
				}
			}
		}

		// restore old input state
		if (gui::visible)
		{
			CopyMemory(&a1->data.buttons, &old_state.buttons, buttons_size);
			CopyMemory(&a1->data.p1_turntable, &old_state.p1_turntable, turntable_size);
		}

		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->INPUT_POLL_FN, reinterpret_cast<LPVOID>(input_hook_fn), (void**) &original_input_fn); }
}