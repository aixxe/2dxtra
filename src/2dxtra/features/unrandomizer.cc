#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <MinHook.h>
#include <safetyhook.hpp>
#include "unrandomizer.h"
#include "../log.h"
#include "../hooks/score_invalidator_hook.h"

namespace iidxtra::unrandomizer
{
	// options
    auto enabled_p1 = false;
    auto enabled_p2 = false;

    auto allow_score_save_p1 = false;
    auto allow_score_save_p2 = false;

	auto show_random_info_p1 = false;
	auto show_random_info_p2 = false;

	auto hran_enabled_p1 = false;
	auto hran_enabled_p2 = false;

	struct random_history
	{
		std::byte unused[0xE8];
		// Timestamp of the most recent group of notes (single note or chord).
		std::int32_t last_group[2];
		// Timestamp of the group before the most recent one.
		std::int32_t previous_group[2];
	};
	static_assert(offsetof(random_history, last_group) == 0xE8);
	static_assert(offsetof(random_history, previous_group) == 0xF0);
	static_assert(sizeof(random_history) == 0xF8);

	static auto hran_hook = SafetyHookMid {};

	// Called inside S-RAN algorithm after it calculates the gap since the candidate lane's last
	// note ended, but before it compares that gap against the minimum allowed gap. By changing
	// this gap value, we can override if this lane should be rejected for note placement.
	//
	// Original S-RAN checks if enough time has passed since the last note in this lane ended.
	//
	// H-RAN additionally tries to avoid recently used lanes, regardless of the time gap.
	// If all candidates are rejected, the game's original S-RAN fallback places the note.
	//
	// ctx.rbp contains the lane number, but it is not checked.
	static auto hran_candidate_hook_fn(SafetyHookContext& ctx) -> void
	{
		// Current player, or side (0 or 1).
		auto const player = static_cast<std::uint8_t>(ctx.r15);

		// If this player / side does not have H-RAN enabled, do nothing and return back to the
		// original S-RAN implementation.
		if (!((player == 0 && hran_enabled_p1) || (player == 1 && hran_enabled_p2)))
			return;

		// Pointer to the random history state.
		// Note that this contains per-player history, not per-lane.
		auto const* history = reinterpret_cast<const random_history*>(ctx.rdi);

		// Timestamp of current note being placed (in # of ticks).
		auto const now = static_cast<std::int32_t>(ctx.r14);

		// Timestamp of the last note, or in the case of charge notes, last release note - in the
		// current candidate lane.
		auto const last_end = static_cast<std::int32_t>(ctx.rbx);

		// Check whether this lane's last note ended at last_group's start timestamp.
		const bool last_chord_had_this_lane = (last_end == history->last_group[player]);

		// Once a note at "now" has been placed, last_group becomes "now" and the timestamp
		// it previously held moves to previous_group. Also compare this lane's end time
		// against that timestamp.
		const bool current_chord_has_notes = (now == history->last_group[player]);
		const bool previous_chord_had_this_lane = (last_end == history->previous_group[player]);

		if (last_chord_had_this_lane ||
			(current_chord_has_notes && previous_chord_had_this_lane))
		{
			// Set the gap (ECX) below the minimum (EAX) so the game rejects this lane for note
			// placement.
			ctx.rcx = static_cast<std::uint32_t>(ctx.rax) - 1u;
		}

		// Ensure that H-RAN disables score saving.
		if (bm2dx::state->play_style == static_cast<int>(bm2dx::play_style::DP))
		{
			// DP
			score_invalidator_hook::invalidate(0);
			score_invalidator_hook::invalidate(1);
		}
		else
		{
			// SP
			score_invalidator_hook::invalidate(player);
		}
	}

	// state
	auto column_lut_p1 = random_lut_t { 0, 1, 2, 3, 4, 5, 6 };
    auto column_lut_p2 = random_lut_t { 0, 1, 2, 3, 4, 5, 6 };

    // original functions
    void* original_game_apply_random_fn = nullptr;

	auto is_valid(const std::uint8_t player) -> bool
	{
		auto seen = std::array<bool, lane_count> {};

		for (auto const i: player == 0 ? column_lut_p1: column_lut_p2)
		{
			if (i >= lane_count)
				return false;

			seen[i] = true;
		}

		return std::ranges::all_of(seen, [] (auto v) { return v; });
	}

    auto reset() -> void
    {
	    auto constexpr lut_default = random_lut_t { 0, 1, 2, 3, 4, 5, 6 };
        enabled_p1 = false;
        enabled_p2 = false;
        allow_score_save_p1 = false;
        allow_score_save_p2 = false;
        show_random_info_p1 = false;
        show_random_info_p2 = false;
        hran_enabled_p1 = false;
        hran_enabled_p2 = false;
		std::ranges::copy(lut_default, column_lut_p1.begin());
		std::ranges::copy(lut_default, column_lut_p2.begin());
    }

	auto apply_force_random(std::uint8_t player) -> void
	{
		// pre-flight checks
		auto const enabled = (player == 0 ? enabled_p1: enabled_p2);
		auto const valid = (enabled && is_valid(player));

		if (!enabled || !valid)
			return;

		// convert textage compatible setting into value expected by the game
		auto const& lut = (player == 0 ? column_lut_p1 : column_lut_p2);

		for (auto i = std::size_t { 0 }; i < lane_count; i++)
		{
			auto const value = lut[i];
			bm2dx::random_data->columns[player][value] = static_cast<std::uint32_t>(i);
		}

        // if player is present, invalidate score
        if (player == 0 && bm2dx::state->p1_active && !allow_score_save_p1)
            score_invalidator_hook::invalidate(0);
        else if (player == 1 && bm2dx::state->p2_active && !allow_score_save_p2)
            score_invalidator_hook::invalidate(1);
	}

	auto game_random_to_string(const std::uint8_t player, random_lut_t& values) -> std::string
	{
		auto result = std::string {};

		for (auto i = std::size_t { 0 }; i < lane_count; i++)
		{
			auto const value = bm2dx::random_data->columns[player][i];

			if (value >= lane_count)
				continue;

			values[value] = static_cast<std::uint8_t>(i);
		}

		for (auto const column: values)
			result.append(std::to_string(column + 1));

		return result;
	}

	auto print_random_info(const std::uint8_t player) -> void
	{
		// get current random as string
		auto translated_random = random_lut_t {};
		auto current = game_random_to_string(player, translated_random);

		// don't display 1234567 (non-ran, s-ran)
		if (current != "1234567")
		{
			auto type = std::string { "?" };

			if (bm2dx::state->play_style == 1)
			{
				type = (player == 0 ? "LEFT": "RIGHT");
				log::print("[Un-randomizer] RANDOM: {} ({})", current, type);
			}
			else
			{
				if (player == 0 && bm2dx::state->p1_active && (show_random_info_p1 || enabled_p1))
					log::print("[Un-randomizer] P1 RANDOM: {}{}", current, (enabled_p1 ? " (CUSTOM)": ""));
				else if (player == 1 && bm2dx::state->p2_active && (show_random_info_p2 || enabled_p2))
					log::print("[Un-randomizer] P2 RANDOM: {}{}", current, (enabled_p2 ? " (CUSTOM)": ""));
			}
		}

		// update lut value to use the random just generated by the game
		// (makes playing or keeping track of good randoms easier)
		if (player == 0 && !enabled_p1)
			std::ranges::copy(translated_random, column_lut_p1.begin());

		if (player == 1 && !enabled_p2)
			std::ranges::copy(translated_random, column_lut_p2.begin());
	}

	void install_hook()
	{
		MH_CreateHook(bm2dx::addr->APPLY_RANDOM_FN, (void*) +[] (void* a1) -> char
		{
			// apply game random immediately
			auto const result = reinterpret_cast<char (*) (void*)>
				(original_game_apply_random_fn) (a1);

			apply_force_random(0);
			apply_force_random(1);

			print_random_info(0);
			print_random_info(1);

			return result;
		}, &original_game_apply_random_fn);

		auto* const candidate_check = bm2dx::addr->SRAN_CANDIDATE_CHECK;
		auto constexpr expected =
			std::array<std::uint8_t, 6> { 0x3B, 0xC8, 0x7C, 0xB6, 0xEB, 0x35 };
		if (std::memcmp(candidate_check, expected.data(), expected.size()) != 0)
		{
			log::print(
				"[Un-randomizer] H-RAN is unavailable: S-RAN candidate check does not match");
			return;
		}

		hran_hook = safetyhook::create_mid(candidate_check, hran_candidate_hook_fn);
		if (!hran_hook)
		{
			log::print("[Un-randomizer] Failed to install H-RAN hook");
			return;
		}
	}
}