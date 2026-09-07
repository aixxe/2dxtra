#include <MinHook.h>
#include "../game.h"
#include "../chart_set.h"
#include "dan_select_hook.h"

namespace iidxtra::dan_select_hook
{
	void* original_dan_select_ctor_fn = nullptr;

	auto dan_select_ctor_hook_fn(void* a1) -> void*
	{
		// Refresh the chart sets from the database.
		chart_set::load_sets();

		// Enable chart set switching.
        chart_set::switch_enabled = true;

		return reinterpret_cast<void* (*) (void*)>(original_dan_select_ctor_fn)(a1);
	}

	void install_hook()
		{ MH_CreateHook(bm2dx::addr->DAN_SELECT_CTOR, reinterpret_cast<LPVOID>(dan_select_ctor_hook_fn), &original_dan_select_ctor_fn); }
}