#include <cstddef>
#include <cstdint>
#include <safetyhook.hpp>
#include "hi_speed_reset.h"
#include "../game.h"

namespace iidxtra::hi_speed_reset
{
    bool enabled = false;

    namespace
    {
        SafetyHookMid start_hook;
        SafetyHookMid adjustment_hook;

        using get_options_fn = void* (*) ();
        using get_style_fn = bool (*) ();
        using get_mode_fn = int (*) (void*, int, int);
        using get_speed_fn = float (*) (void*, int, int);
        using set_speed_fn = void (*) (void*, unsigned int, float);

        auto active(int player) -> bool
        {
            if (!enabled || !bm2dx::play_session->ready)
                return false;

            auto* options = reinterpret_cast<get_options_fn>(bm2dx::addr->GET_PLAYER_OPTIONS)();
            auto const style = reinterpret_cast<get_style_fn>(bm2dx::addr->IS_DOUBLE_PLAY)();

            return reinterpret_cast<get_mode_fn>(bm2dx::addr->GET_HI_SPEED_MODE)(options, player, style) == 1;
        }

        auto double_start(SafetyHookContext& ctx) -> void
        {
            auto* control = reinterpret_cast<bm2dx::lane_cover_control*>(ctx.rsi);

            if (!active(control->player))
                return;

            auto* options = reinterpret_cast<get_options_fn>(bm2dx::addr->GET_PLAYER_OPTIONS)();
            auto const style = reinterpret_cast<get_style_fn>(bm2dx::addr->IS_DOUBLE_PLAY)();
            auto const speed = reinterpret_cast<get_speed_fn>(bm2dx::addr->GET_SAVED_HI_SPEED)(options, control->player, style);

            reinterpret_cast<set_speed_fn>(bm2dx::addr->SET_HI_SPEED)(bm2dx::addr->HI_SPEED_STATE, control->player, speed);
            control->double_tap_frames = 0;

            // preserve cover state, then use the stock lane/green-number
            // recomputation and option writeback. game owns double-tap timing.
            ctx.rip = reinterpret_cast<std::uintptr_t>(bm2dx::addr->HI_SPEED_RESET_CONTINUE);
        }

        auto adjust(SafetyHookContext& ctx) -> void
        {
            auto const* control = reinterpret_cast<const bm2dx::lane_cover_control*>(ctx.rcx);

            if (active(control->player))
                ctx.rip += 5; // skip the call
        }
    }

    auto reset() -> void
    {
        enabled = false;
    }

    auto install_hook() -> void
    {
        start_hook = safetyhook::create_mid(bm2dx::addr->HI_SPEED_RESET_PATCH, double_start);
        adjustment_hook = safetyhook::create_mid(bm2dx::addr->HI_SPEED_ADJUST_CALL, adjust);
    }
}
