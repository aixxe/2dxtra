#include "../game.h"
#include "../chart_set.h"
#include "../util/scoped_page_permissions.h"
#include "card_out_hook.h"

class CCardOutScene;

namespace iidxtra::card_out_hook
{
	void* original_card_out_fn = nullptr;

	auto card_out_hook_fn(CCardOutScene* thisptr) -> void*
	{
		// switch back to default scores before we card out, this is just to correct our DJ points
        // reset_state_hook will also call this in case the user test menus out
		chart_set::revert();

		return reinterpret_cast<void* (*) (CCardOutScene*)>(original_card_out_fn)(thisptr);
	}

	auto install_hook() -> void
	{
		auto guard = util::scoped_page_permissions { bm2dx::addr->CARD_OUT_VFUNC, 8, PAGE_EXECUTE_READWRITE };
		auto const target = reinterpret_cast<void**>(bm2dx::addr->CARD_OUT_VFUNC);

		// store pointer to original function
		original_card_out_fn = *target;

		// now overwrite with our hook function
		*target = card_out_hook_fn;
	}

	auto uninstall_hook() -> void
	{
		auto guard = util::scoped_page_permissions { bm2dx::addr->CARD_OUT_VFUNC, 8, PAGE_EXECUTE_READWRITE };
		auto const target = reinterpret_cast<void**>(bm2dx::addr->CARD_OUT_VFUNC);

		*target = original_card_out_fn;
	}
}