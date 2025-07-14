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

	auto input_hook_fn(bm2dx::InputManagerIIDX* a1) -> void*
	{
		auto static timeout = 0;
		auto static old_state = bm2dx::input_t {};

		// menu is visible -- lock inputs from the game
		if (gui::visible)
			CopyMemory(&old_state, &a1->data, sizeof(a1->data));

		// get the new inputs and feed them to the menu
		auto const result = original_input_fn(a1);

		if (gui::visible)
			CopyMemory(&input::menu, &a1->data, sizeof(a1->data));

		// toggle the gui state
		{
			if (timeout > 0)
				timeout--;

			if (std::bitset<32>(a1->data.buttons).test(18))
			{
				if (timeout > 0)
				{
					// second tap occurred within a second
					// check if we're allowed to open the gui
					if (!gui::visible && gui::play_lock_state && *bm2dx::in_gameplay)
						log::print("Menu is currently unavailable");
					else
						gui::visible = !gui::visible;

					timeout = 0;
				}
				else
				{
					// now waiting for the next tap
					timeout = (bm2dx::config->target_fps * 0.2);
				}
			}
		}

		// restore old input state
		if (gui::visible)
			CopyMemory(&a1->data, &old_state, sizeof(a1->data));

		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->INPUT_POLL_FN, input_hook_fn, (void**) &original_input_fn); }
}