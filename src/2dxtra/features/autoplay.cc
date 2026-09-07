#include <safetyhook.hpp>
#include "autoplay.h"
#include "../game.h"
#include "../hooks/score_invalidator_hook.h"

namespace iidxtra::autoplay
{
    auto btn_down_hook_a = SafetyHookMid {};
    auto btn_down_hook_b = SafetyHookMid {};
    auto lane_beam_hook = SafetyHookMid {};
    auto lane_beam_player_hook = SafetyHookMid {};

    auto enabled_p1 = false;
    auto enabled_p2 = false;

    // Which player the lane beam renderer is currently drawing for. The check
    // we hook sits inside a per-lane loop with the player only reachable
    // through the renderer's argument, so it is captured on the way in.
    auto lane_beam_player = 0;

    // `cmp byte ptr [rax+disp32], 0` - skipping it leaves the following `jcc`
    // to act on the flags we set instead.
    auto constexpr autoplay_check_length = 7;
    auto constexpr flag_zero = 0x40ull;

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

    auto force_autoplay_branch(SafetyHookContext& ctx) -> void
    {
        ctx.rip += autoplay_check_length;
        ctx.rflags &= ~flag_zero;
    }

    auto is_btn_down_hook(SafetyHookContext& ctx)
    {
        if (state_check_common(static_cast<int>(ctx.rdx)))
            force_autoplay_branch(ctx);
    }

    auto auto_beam_hook(SafetyHookContext& ctx)
    {
    	if (state_check_common(lane_beam_player))
            force_autoplay_branch(ctx);
    }

    void install_hook()
    {
        btn_down_hook_a = create_mid(bm2dx::addr->IS_BTN_DOWN_FN_A, is_btn_down_hook);
        btn_down_hook_b = create_mid(bm2dx::addr->IS_BTN_DOWN_FN_B, is_btn_down_hook);
        lane_beam_hook = create_mid(bm2dx::addr->AUTO_BEAM_PATCH, auto_beam_hook);
        lane_beam_player_hook = create_mid(bm2dx::addr->AUTO_BEAM_FN,
            +[] (SafetyHookContext& ctx)
        {
            // rcx is the renderer's per-player block; its first dword is the
            // player index the rest of the routine indexes everything with.
            if (ctx.rcx)
                lane_beam_player = *reinterpret_cast<std::uint32_t*>(ctx.rcx);
        });
	}
}