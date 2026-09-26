#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>
#include <string_view>
#include <safetyhook.hpp>
#include "play_option.h"
#include "unrandomizer.h"
#include "../game.h"
#include "../log.h"

namespace iidxtra::play_option
{
	static auto result_label_hook = SafetyHookInline {};
	static auto retry_label_hook = SafetyHookInline {};
	static auto pacemaker_label_hook = SafetyHookInline {};

	struct option_text
	{
		union
		{
			char local[16];
			char* heap;
		};
		std::size_t length;
		std::size_t capacity;
	};
	static_assert(sizeof(option_text) == 0x20);
	static_assert(offsetof(option_text, length) == 0x10);
	static_assert(offsetof(option_text, capacity) == 0x18);

	static auto replace_label_if_equal(std::span<char> label, std::string_view original,
		std::string_view replacement) -> void
	{
		if (!std::equal(label.begin(), label.end(), original.begin(), original.end()))
			return;

		if (original.size() == replacement.size())
			std::copy(replacement.begin(), replacement.end(), label.begin());
	}

	static auto replace_sran_with_hran(option_text* text, unsigned int player) -> void
	{
		if (!unrandomizer::hran_enabled_p1 && !unrandomizer::hran_enabled_p2)
			return;

		// Defensive checks.
		if (text == nullptr || text->length > text->capacity ||
			(text->capacity >= sizeof(text->local) && text->heap == nullptr))
		{
			return;
		}

		auto* const buffer = text->capacity < sizeof(text->local) ? text->local : text->heap;
		auto const options = std::span<char> { buffer, text->length };

		auto split_options = options | std::views::split(',');
		for (auto option_range: split_options)
		{
			// Create a writable view as a span of characters.
			auto option = std::span<char> { option_range };

			// Skip leading spaces, as the game inserts one after each comma.
			while (!option.empty() && option.front() == ' ')
				option = option.subspan(1);

			// DP random options contain a slash; e.g., "R-RAN/S-RAN"
			auto const separator = std::string_view { option.data(), option.size() }.find('/');
			if (separator == std::string_view::npos)
			{
				// SP: check if the current player has H-RAN enabled.
				auto const sp_player_has_hran =
					(player == 0 && unrandomizer::hran_enabled_p1) ||
					(player == 1 && unrandomizer::hran_enabled_p2);
				if (sp_player_has_hran)
					replace_label_if_equal(option, "S-RANDOM", "H-RANDOM");
			}
			else
			{
				// DP: check both hands.
				if (unrandomizer::hran_enabled_p1)
					replace_label_if_equal(option.first(separator), "S-RAN", "H-RAN");
				if (unrandomizer::hran_enabled_p2)
					replace_label_if_equal(option.subspan(separator + 1), "S-RAN", "H-RAN");
			}
		}
	}

	static auto result_label_hook_fn(void* options, option_text* text, unsigned int player,
		unsigned int style, char spaced) -> option_text*
	{
		auto* const formatted_text =
			result_label_hook.call<option_text*>(options, text, player, style, spaced);
		replace_sran_with_hran(formatted_text, player);
		return formatted_text;
	}

	static auto retry_label_hook_fn(void* options, option_text* text, unsigned int player,
		unsigned int style, char spaced) -> option_text*
	{
		auto* const formatted_text =
			retry_label_hook.call<option_text*>(options, text, player, style, spaced);
		replace_sran_with_hran(formatted_text, player);
		return formatted_text;
	}

	static auto pacemaker_label_hook_fn(option_text* text) -> option_text*
	{
		auto* const formatted_text = pacemaker_label_hook.call<option_text*>(text);
		auto const player = (!bm2dx::state->p1_active && bm2dx::state->p2_active) ? 1u : 0u;
		replace_sran_with_hran(formatted_text, player);
		return formatted_text;
	}

	auto install_hook() -> void
	{
		result_label_hook =
			safetyhook::create_inline(bm2dx::addr->RESULT_OPTIONS_FN, result_label_hook_fn);
		retry_label_hook =
			safetyhook::create_inline(bm2dx::addr->RETRY_OPTIONS_FN, retry_label_hook_fn);
		pacemaker_label_hook =
			safetyhook::create_inline(bm2dx::addr->PACEMAKER_OPTIONS_FN, pacemaker_label_hook_fn);

		if (!result_label_hook || !retry_label_hook || !pacemaker_label_hook)
		{
			log::print("[Play options] Failed to install label hooks");
			result_label_hook.reset();
			retry_label_hook.reset();
			pacemaker_label_hook.reset();
		}
	}
}
