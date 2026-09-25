#include <bitset>
#include <chrono>
#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../input.h"
#include "../gui/gui.h"
#include "input_hook.h"

namespace iidxtra::input_hook
{
	void* (*original_input_fn) (bm2dx::InputManagerIIDX*) = nullptr;

	// How long the second EFFECT tap may arrive after the first, in milliseconds.
	auto constexpr double_tap_window = std::chrono::milliseconds { 200 };

	auto input_hook_fn(bm2dx::InputManagerIIDX* a1) -> void*
	{
		using clock = std::chrono::steady_clock;
		auto static timeout = clock::now();

		// Not zero-initialized here; will be overwritten every poll
		bm2dx::input_t old_state;

		// The poll fn's input data lives in two disjoint regions of the object:
		// the button bitfields at the front and the turntable slots further in.
		// The bytes between them are the game's own list/map headers - copying
		// those back corrupts the input manager and crashes on the next poll,
		// so only the two named windows are ever touched.
		auto constexpr buttons_size = bm2dx::INPUT_BUTTON_BYTES;
		auto constexpr turntable_size = bm2dx::INPUT_TURNTABLE_BYTES;

		auto const effect_bit = static_cast<std::size_t>(bm2dx::button::EFFECT);

		CopyMemory(&old_state.buttons, &a1->data.buttons, buttons_size);
		CopyMemory(&old_state.p1_turntable, &a1->data.p1_turntable, turntable_size);

		// get the new inputs and feed them to the menu
		auto const result = original_input_fn(a1);

		if (gui::visible)
		{
			CopyMemory(&input::menu.buttons, &a1->data.buttons, buttons_size);
			CopyMemory(&input::menu.p1_turntable, &a1->data.p1_turntable, turntable_size);
		}

		// toggle the gui state
		{
			if (std::bitset<32>(a1->data.buttons).test(effect_bit))
			{
				auto const now = clock::now();
				if (now < timeout)
				{
					// second tap occurred within the window
					// check if we're allowed to open the gui
					if (!gui::visible && gui::play_lock_state && bm2dx::play_session->in_gameplay)
						log::print("Menu is currently unavailable");
					else
						gui::visible = !gui::visible;

					timeout = now;
				}
				else
				{
					// now waiting for the next tap
					timeout = now + double_tap_window;
				}
			}
		}

		// restore old input state
		if (gui::visible)
		{
			CopyMemory(&a1->data.buttons, &old_state.buttons, buttons_size);
			CopyMemory(&a1->data.p1_turntable, &old_state.p1_turntable, turntable_size);
			a1->data.p1_turntable_delta = 0;
			a1->data.p2_turntable_delta = 0;
		}

		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->INPUT_POLL_FN, reinterpret_cast<LPVOID>(input_hook_fn), (void**) &original_input_fn); }
}