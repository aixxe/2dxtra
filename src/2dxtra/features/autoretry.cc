#include <safetyhook.hpp>
#include "autoretry.h"
#include "../game.h"
#include "../log.h"

namespace iidxtra::autoretry
{
    auto enabled = false;

    auto target = target_mode::Off;
    auto destination = target_destination::Graph;

    auto retrying = false;

    auto graph_text_hook = SafetyHookMid {};
    auto ghost_text_hook = SafetyHookMid {};
    auto graph_condition_hook = SafetyHookMid {};
    auto retry_btn_check_a_hook = SafetyHookMid {};
    auto retry_btn_check_b_hook = SafetyHookMid {};
    auto play_failed_animation_hook = SafetyHookMid {};
    auto mute_failed_sound_hook = SafetyHookMid {};
    auto failed_transition_hook = SafetyHookMid {};

    // x86 flag bits the mid-function hooks steer the following branch with.
    auto constexpr flag_zero = 1ull << 6;
    auto constexpr flag_overflow = 1ull << 11;

    // `cmp byte ptr [rbp+disp32], 0` guarding the TARGET text.
    auto constexpr graph_condition_length = 7;

    // `call rel32` testing a button; skipping it lets us answer instead.
    auto constexpr button_check_length = 5;

    auto reset() -> void
        { enabled = false; }

    auto inline get_max_score(
        const std::int32_t current_note,
        const std::int32_t total_notes,
        const std::int32_t ex_score
    ) { return ex_score + (total_notes > current_note ? (total_notes - current_note) * 2: 0); }

    auto text_render_hook(const target_destination type, std::uintptr_t& result) -> void
    {
        auto const* const dead = bm2dx::dead_state;
        auto& player = bm2dx::play_state->players[!dead->p1 ? 0: 1];

        // Pacemaker score target is not set if the 'MY BEST' type is used.
        // In this case, we'll use the current score PB instead.
        auto score = player.ex_score;

        if (bm2dx::play_session->pacemaker_type_id == bm2dx::pacemaker_type::MY_BEST)
            score = static_cast<std::int32_t>(bm2dx::play_session->current_score_pb);

        // Given our current progress in the chart, calculate the best possible score we can get.
        auto const best_score = get_max_score(player.note_current, player.note_total, score);
        auto const score_target = static_cast<std::int32_t>(bm2dx::play_state->pacemaker_target);

        // Update the target text in the graph.
        if (type == destination && target != target_mode::Off)
        {
            if (target == target_mode::Delta)
                result = static_cast<std::uintptr_t>(best_score - score_target);
            else if (target == target_mode::Maximum)
                result = static_cast<std::uintptr_t>(best_score);
        }

        // Auto Retry stuff from this point onwards.
        if (!enabled || retrying || (dead->p1 && dead->p2))
            return;

        // If the target is above this, we can no longer clear.
        // Set the auto-retry flag and fail the stage.
        if (best_score < score_target)
        {
            log::debug("Initiating auto retry...");
            retrying = true;
            bm2dx::dead_state->p1 = true;
            bm2dx::dead_state->p2 = true;
        }
    }

    auto install_hook() -> void
    {
        // Handle the core functionality, as well as altering the target or ghost text.
        graph_text_hook = safetyhook::create_mid(bm2dx::addr->GHOST_TARGET_FN,
            [] (SafetyHookContext& ctx) { text_render_hook(target_destination::Ghost, ctx.r8); });
        ghost_text_hook = safetyhook::create_mid(bm2dx::addr->GRAPH_TARGET_FN,
            [] (SafetyHookContext& ctx) { text_render_hook(target_destination::Graph, ctx.r8); });

        // Ensure that the graph target text is rendered if it meets the necessary conditions.
        graph_condition_hook = safetyhook::create_mid(bm2dx::addr->GRAPH_CONDITION,
            [] (SafetyHookContext& ctx)
        {
            if (target != target_mode::Off && destination == target_destination::Graph)
            {
                ctx.rip += graph_condition_length;
                ctx.rflags &= ~flag_zero;
            }
        });

        // First hook where the game checks if EFFECT is held.
        // If we're auto-retrying, simulate the button being held.
        retry_btn_check_a_hook = safetyhook::create_mid(bm2dx::addr->RETRY_CHECK_A,
            [] (SafetyHookContext& ctx)
        {
            if (retrying)
            {
                ctx.rip += button_check_length;
                ctx.rax = 1;
            }
        });

        // Second hook where the game checks if VEFX is held.
        // After this is called, an attempt will be made to quick retry.
        retry_btn_check_b_hook = safetyhook::create_mid(bm2dx::addr->RETRY_CHECK_B,
            [] (SafetyHookContext& ctx)
        {
            if (retrying)
            {
                ctx.rip += button_check_length;
                ctx.rax = 1;

                retrying = false;
            }
        });

        // Prevent the 'stage failed' animation from being played when auto-retrying.
        play_failed_animation_hook = safetyhook::create_mid(bm2dx::addr->FAIL_ANIMATION_FN,
            [] (SafetyHookContext& ctx) { ctx.rdx = !retrying; });

        // Prevent the 'stage failed' system sound from being played when auto-retrying.
        mute_failed_sound_hook = safetyhook::create_mid(bm2dx::addr->FAIL_PLAY_SFX_FN,
            [] (SafetyHookContext& ctx) { ctx.rcx = retrying ? 0: ctx.rcx; });

        // Skip the 'stage failed' animation and fade out immediately when
        // auto-retrying. The branch below is a `jl` on the elapsed animation
        // time, so setting OF flips it into the "already finished" path.
        failed_transition_hook = safetyhook::create_mid(bm2dx::addr->FAIL_DURATION_JMP,
            [] (SafetyHookContext& ctx)
        {
            if (retrying)
                ctx.rflags = (ctx.rflags | flag_overflow) & ~flag_zero;
        });
    }
}