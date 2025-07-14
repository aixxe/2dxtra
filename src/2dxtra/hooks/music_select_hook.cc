#include <MinHook.h>
#include <safetyhook.hpp>
#include "../game.h"
#include "../chart_set.h"
#include "../score_set.h"
#include "music_select_hook.h"

namespace iidxtra::music_select_hook
{
	void* original_music_select_ctor_fn = nullptr;
    auto scene_dtor_mid_fn_hook = safetyhook::MidHook {};

	auto music_select_ctor_hook_fn(void* a1, int a2) -> void*
	{
		// Load scores from the current chart set.
		score_set::reload_all();

		// Enable chart set switching.
        chart_set::switch_enabled = true;

		return reinterpret_cast<void* (*) (void*, int)>(original_music_select_ctor_fn)(a1, a2);
	}

    auto scene_dtor_hook_fn(SafetyHookContext&) -> void
    {
		// Chart switching was enabled when entering a music select scene.
        // It should be disabled as soon as the scene is changed to anything else.
        if (!chart_set::switch_enabled)
            return;

        chart_set::switch_enabled = false;
	}

	auto install_hook() -> void
	{
		MH_CreateHook(bm2dx::addr->MUSIC_SELECT_CTOR, music_select_ctor_hook_fn, &original_music_select_ctor_fn);
        scene_dtor_mid_fn_hook = safetyhook::create_mid(bm2dx::addr->SCENE_DTOR, scene_dtor_hook_fn);
	}
}