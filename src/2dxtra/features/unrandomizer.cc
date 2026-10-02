#include <algorithm>
#include <cstddef>
#include <MinHook.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include "unrandomizer.h"
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

	static auto game_random_to_lanes(const std::uint8_t player, random_lut_t& values) -> void
	{
		for (auto i = std::size_t { 0 }; i < lane_count; i++)
		{
			auto const value = bm2dx::random_data->columns[player][i];

			if (value >= lane_count)
				continue;

			values[value] = static_cast<std::uint8_t>(i);
		}
	}

	struct player_option_block
	{
		std::byte unused[0x30];
		std::int32_t random[2];
		std::int32_t mirror[2];
		std::byte unused_tail[0x74];
	};
	static_assert(offsetof(player_option_block, random) == 0x30);
	static_assert(offsetof(player_option_block, mirror) == 0x38);
	static_assert(sizeof(player_option_block) == 0xB4);

	struct player_options
	{
		std::byte unused[8];
		player_option_block blocks[3]; // P1, P2, DP, in that order
	};
	static_assert(offsetof(player_options, blocks) == 8);

	struct random_sequence_data
	{
		std::byte unused[0x58];
		// per-side display state: 0 = hidden, 1 = success, 2 = failure.
		std::int32_t status[2];
		// per-side seven-digit lane order, e.g. 2741635.
		std::int32_t random[2];
	};
	static_assert(offsetof(random_sequence_data, status) == 0x58);
	static_assert(offsetof(random_sequence_data, random) == 0x60);

	static auto original_random_ticket_data_fn = static_cast<random_sequence_data*(*)()>(nullptr);

	static auto random_ticket_data_hook_fn() -> random_sequence_data*
	{
		auto* result = original_random_ticket_data_fn();

		// Determine if the random sequence display should be shown.
		auto const show_sequence_p1 = show_random_info_p1 || enabled_p1;
		auto const show_sequence_p2 = show_random_info_p2 || enabled_p2;
		if (!show_sequence_p1 && !show_sequence_p2)
			return result;

		// Sanity check.
		if (result == nullptr || bm2dx::state == nullptr || bm2dx::random_data == nullptr)
			return result;

		// Look up the caller address.
#ifdef _MSC_VER
		auto const caller = _ReturnAddress();
#else
		auto const caller = __builtin_return_address(0);
#endif

		DWORD64 image_base = 0;
		auto const function =
			RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(caller), &image_base, nullptr);
		if (function == nullptr)
			return result;

		// Check that the caller is the Random Sequence draw function.
		if (image_base + function->BeginAddress !=
			reinterpret_cast<DWORD64>(bm2dx::addr->RANDOM_SEQUENCE_DRAW_FN))
			return result;

		static thread_local auto display = random_sequence_data {};
		display = *result;

		// For SP, setting is enabled per-player.
		// For DP, having it enabled for either side enables both sides.
		auto const dp = bm2dx::state->play_style == static_cast<int>(bm2dx::play_style::DP);
		auto const show_player = std::array {
			dp || (show_sequence_p1 && bm2dx::state->p1_active),
			dp || (show_sequence_p2 && bm2dx::state->p2_active)
		};
		auto const* options =
			reinterpret_cast<player_options* (*)()>(bm2dx::addr->GET_PLAYER_OPTIONS)();

		for (std::uint8_t player = 0; player < 2; ++player)
		{
			if (!show_player[player])
				continue;

			// If random ticket is in use for this player, Random Sequence display should already
			// be visible (set to something other than 0 / hidden).
			if (result->status[player] != 0)
				continue;

			auto const& block = options->blocks[dp ? 2 : player];
			auto const random = block.random[player];
			auto const mirror = block.mirror[player];

			// Never show for S-RAN
			if (random == 3) 
				continue;
			// Never show for non-ran
			if (random == 0 && mirror == 0)
				continue;

			auto lanes = random_lut_t {};
			game_random_to_lanes(player, lanes);
			display.status[player] = 1; // Success
			display.random[player] = 0;
			for (auto const lane: lanes)
				display.random[player] = display.random[player] * 10 + lane + 1;
		}

		return &display;
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

			if (!enabled_p1)
				game_random_to_lanes(0, column_lut_p1);
			if (!enabled_p2)
				game_random_to_lanes(1, column_lut_p2);

			return result;
		}, &original_game_apply_random_fn);

		MH_CreateHook(bm2dx::addr->RANDOM_TICKET_DATA_FN,
			reinterpret_cast<LPVOID>(random_ticket_data_hook_fn),
			reinterpret_cast<LPVOID*>(&original_random_ticket_data_fn));
	}
}