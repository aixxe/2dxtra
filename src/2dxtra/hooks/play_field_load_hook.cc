#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../features/cn_override.h"
#include "play_field_load_hook.h"

namespace iidxtra::play_field_load_hook
{
	void* original_play_field_load_fn = nullptr;

	auto play_field_load_hook_fn(bm2dx::play_field_t* play_field, void* a2, void* a3, int a4) -> void*
	{
        // Call the original first.
        auto const result = reinterpret_cast<void* (*) (bm2dx::play_field_t*, void*, void*, int)>(original_play_field_load_fn)(play_field, a2, a3, a4);

        // Update the play field pointer.
        bm2dx::play_field = play_field;

		// Set the initial CN states.
        if (bm2dx::state->p1_active || bm2dx::state->play_style == 1) cn_override::set_initial_states(0);
        if (bm2dx::state->p2_active || bm2dx::state->play_style == 1) cn_override::set_initial_states(1);

        // Now let it be overridden.
        cn_override::update(0);
        cn_override::update(1);

        return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->PLAY_FIELD_LOAD, play_field_load_hook_fn, &original_play_field_load_fn); }
}