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

    auto reset() -> void
        { enabled = false; }

    auto inline get_max_score(
        const std::uint32_t current_note,
        const std::uint32_t total_notes,
        const std::uint32_t ex_score
    ) { return ex_score + (total_notes - current_note) * 2; }

    auto text_render_hook(const target_destination type, std::uintptr_t& result) -> void
    {
        auto static dead_state = reinterpret_cast<bm2dx::dead_state_t*>(bm2dx::addr->DEAD_STATE);
        auto static pacemaker_score_target = reinterpret_cast<std::uint32_t*>(bm2dx::addr->PACEMAKER_TARGET);
        auto static p1_score_current = reinterpret_cast<std::uint32_t*>(bm2dx::addr->SCORE_CURRENT_P1);
        auto static p2_score_current = reinterpret_cast<std::uint32_t*>(bm2dx::addr->SCORE_CURRENT_P2);
        auto static p1_note_current = reinterpret_cast<std::uint32_t*>(bm2dx::addr->NOTE_CURRENT_P1);
        auto static p2_note_current = reinterpret_cast<std::uint32_t*>(bm2dx::addr->NOTE_CURRENT_P2);
        auto static p1_note_total = reinterpret_cast<std::uint32_t*>(bm2dx::addr->NOTE_TOTAL_P1);
        auto static p2_note_total = reinterpret_cast<std::uint32_t*>(bm2dx::addr->NOTE_TOTAL_P2);
        auto static pacemaker_type_id = reinterpret_cast<bm2dx::pacemaker_type*>(bm2dx::addr->PACEMAKER_TYPE_ID);
        auto static current_score_pb = reinterpret_cast<std::uint32_t*>(bm2dx::addr->CURRENT_SCORE_PB);

        // Player-specific variables.
        auto const note_current = !dead_state->p1 ? *p1_note_current: *p2_note_current;
        auto const note_total = !dead_state->p1 ? *p1_note_total: *p2_note_total;

        // Pacemaker score target is not set if the 'MY BEST' type is used.
        // In this case, we'll use the current score PB instead.
        auto score = !dead_state->p1 ? *p1_score_current: *p2_score_current;

        if (*pacemaker_type_id == bm2dx::pacemaker_type::MY_BEST)
            score = *current_score_pb;

        // Given our current progress in the chart, calculate the best possible score we can get.
        auto const best_score = get_max_score(note_current, note_total, score);

        // Update the target text in the graph.
        if (type == destination && target != target_mode::Off)
        {
            if (target == target_mode::Delta)
                result = -(*pacemaker_score_target - best_score);
            else if (target == target_mode::Maximum)
                result = best_score;
        }

        // Auto Retry stuff from this point onwards.
        if (!enabled || retrying || (dead_state->p1 && dead_state->p2))
            return;

        // If the target is above this, we can no longer clear.
        // Set the auto-retry flag and fail the stage.
        if (best_score < *pacemaker_score_target)
        {
            log::debug("Initiating auto retry...");
            retrying = true;
            dead_state->p1 = true;
            dead_state->p2 = true;
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
                ctx.rip += 7;
                ctx.rflags &= ~0x40;
            }
        });

        // First hook where the game checks if EFFECT is held.
        // If we're auto-retrying, simulate the button being held.
        retry_btn_check_a_hook = safetyhook::create_mid(bm2dx::addr->RETRY_CHECK_A,
            [] (SafetyHookContext& ctx)
        {
            if (retrying)
            {
                ctx.rip += 5;
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
                ctx.rip += 5;
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

        // Skip the 'stage failed' animation and fade out immediately when auto-retrying.
        failed_transition_hook = safetyhook::create_mid(bm2dx::addr->FAIL_DURATION_JMP,
            [] (SafetyHookContext& ctx)
        {
            if (retrying)
                ctx.rflags = (ctx.rflags | 1ULL << 11) & ~(1ULL << 6);
        });
    }
}