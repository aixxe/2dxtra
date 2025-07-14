#include <MinHook.h>
#include "../game.h"
#include "../features/timing_modifier.h"
#include "judge_timing_hook.h"

namespace iidxtra::judge_timing_hook
{
	void* original_set_timing_fn = nullptr;

	auto set_timing_hook_fn(bm2dx::timing_data_t* data, int player, int a3, int a4) -> void*
	{
        // Call the original first.
        auto const result = reinterpret_cast<void* (*) (void*, int, int, int)>
			(original_set_timing_fn) (data, player, a3, a4);

        // Modify the timing windows.
        timing_modifier::execute(data, player);

        return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->TIMING_HOOK_FN, set_timing_hook_fn, &original_set_timing_fn); }
}