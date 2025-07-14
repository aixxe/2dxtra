#include <safetyhook.hpp>
#include "autoplay.h"
#include "../game.h"
#include "../hooks/score_invalidator_hook.h"

namespace iidxtra::autoplay
{
    auto btn_down_hook_a = SafetyHookMid {};
    auto btn_down_hook_b = SafetyHookMid {};
    auto lane_beam_hook = SafetyHookMid {};

    auto enabled_p1 = false;
    auto enabled_p2 = false;

    auto reset() -> void
    {
        enabled_p1 = false;
        enabled_p2 = false;
    }

	auto state_check_common(const int player)
    {
    	auto static global_active = reinterpret_cast<bool*>(bm2dx::addr->AUTO_PLAY);

    	auto const p1_active = player == 0 && enabled_p1;
		auto const p2_active = player == 1 && enabled_p2;

	    if (p1_active || p2_active)
		{
			if (bm2dx::state->play_style == 1)
			{
				score_invalidator_hook::invalidate(0);
				score_invalidator_hook::invalidate(1);
			}
			else
			{
				score_invalidator_hook::invalidate(player);
			}
		}

	    return p1_active || p2_active || *global_active;
    }

    auto is_btn_down_hook(SafetyHookContext& ctx)
    {
        if (state_check_common(static_cast<int>(ctx.rdx)))
    	{
    		ctx.rip += 7;
            ctx.rflags &= ~0x40; // clear ZF
    	}
    }

    auto auto_beam_hook(SafetyHookContext& ctx)
    {
    	auto const player = *reinterpret_cast<std::uint32_t*>
    		(ctx.rsp + 0x170) == 2 ? 0: 1;

    	if (state_check_common(player))
    	{
    		ctx.rip += 7;
            ctx.rflags &= ~0x40; // clear ZF
    	}
    }

    void install_hook()
    {
        btn_down_hook_a = create_mid(bm2dx::addr->IS_BTN_DOWN_FN_A, is_btn_down_hook);
        btn_down_hook_b = create_mid(bm2dx::addr->IS_BTN_DOWN_FN_B, is_btn_down_hook);
        lane_beam_hook = create_mid(bm2dx::addr->AUTO_BEAM_PATCH, auto_beam_hook);
	}
}