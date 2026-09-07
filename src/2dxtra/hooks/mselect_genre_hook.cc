#include <MinHook.h>
#include <safetyhook.hpp>
#include <fmt/format.h>
#include "../game.h"
#include "../input.h"
#include "../chart_set.h"
#include "mselect_genre_hook.h"

namespace iidxtra::mselect_genre_hook
{
	void* original_mselect_render_fn = nullptr;

	auto genre_text_hook = SafetyHookMid {};
	auto genre_texture_hook = SafetyHookMid {};

	bm2dx::music_entry_t* music = nullptr;
	bool should_render_genre_texture = false;

	std::string genre_string = {};
	std::wstring genre_wide = {};

	// `cmp dword ptr [rax+texture_genre], 0`; skipping it hands the following
	// `je` the flags we set instead.
	auto constexpr texture_check_length = 7;
	auto constexpr flag_zero = 0x40ull;

	// Sixth argument of the genre text call: shadow space (0x20) plus the
	// fifth argument (0x8).
	auto constexpr text_argument_offset = 0x28;

	/**
	 * Utility function for generating the formatted genre text string.
	 */
	auto append_genre_string(std::string& text, std::unordered_map<std::uint8_t, chart_set::chart_t>& original_charts,
		std::unordered_map<std::uint8_t, chart_set::chart_t>& custom_charts, std::uint32_t chart_id,
		char prefix, const char* color, bool show_delta) -> void
	{
		if (!custom_charts.contains(chart_id) || !original_charts.contains(chart_id))
			return;

		auto const original = original_charts.at(chart_id).notes;
		auto const custom = custom_charts.at(chart_id).notes;

		if (custom == original)
			return;

		auto delta = static_cast<std::int32_t>(custom - original);
		auto delta_str = (delta > 0 ? "+": "");

		if (!show_delta) {
			delta_str = "=";
			delta = custom;
		}

		text.append(fmt::format("{}<color {}>{}{}</color> ", prefix, color, delta_str, delta));
	}

	/**
	 * Outer function hook used to get the highlighted music entry.
	 * Also used to determine whether we should draw custom genre text.
	 */
	auto mselect_render_hook(void* a1) -> void
	{
		auto static original_fn = reinterpret_cast<void (*) (void*)>(original_mselect_render_fn);

		// Get the currently hovered music entry.
		struct select_data_t
		{
			char pad_0000[16]; //0x0000
			bm2dx::music_entry_t* music; //0x0010
		}; static_assert(sizeof(select_data_t) == 0x18);

		music = static_cast<select_data_t*>(a1)->music;

		if (!music)
			return original_fn(a1);

		// Set default state for rendering of genre textures.
		// Essentially this is the game default behaviour and we're free to override it later on.
		should_render_genre_texture = (music->texture_genre != 0);

		// Generate the custom genre text string for when a custom chart set is in use.
		genre_string.clear();

		if (!chart_set::active.empty())
		{
			auto& active_set = chart_set::custom.at(chart_set::active);
			auto show_delta = !input::test_game_button(bm2dx::button::EFFECT);

			if (active_set.music.contains(music->id) && chart_set::stock.music.contains(music->id))
			{
				auto& original_charts = chart_set::stock.music.at(music->id).charts;
				auto& custom_charts = active_set.music.at(music->id).charts;

				if (bm2dx::state->play_style == 0)
				{
					append_genre_string(genre_string, original_charts, custom_charts, 3, 'B', "70ff58ff", show_delta); // SPB
					append_genre_string(genre_string, original_charts, custom_charts, 1, 'N', "58baffff", show_delta); // SPN
					append_genre_string(genre_string, original_charts, custom_charts, 0, 'H', "ffb658ff", show_delta); // SPH
					append_genre_string(genre_string, original_charts, custom_charts, 2, 'A', "ff5858ff", show_delta); // SPA
					append_genre_string(genre_string, original_charts, custom_charts, 4, 'L', "b658ffff", show_delta); // SPL
				}
				else
				{
					append_genre_string(genre_string, original_charts, custom_charts, 7, 'N', "58baffff", show_delta); // DPN
					append_genre_string(genre_string, original_charts, custom_charts, 6, 'H', "ffb658ff", show_delta); // DPH
					append_genre_string(genre_string, original_charts, custom_charts, 8, 'A', "ff5858ff", show_delta); // DPA
					append_genre_string(genre_string, original_charts, custom_charts, 10, 'L', "b658ffff", show_delta); // DPL
				}
			}

			if (should_render_genre_texture && !genre_string.empty())
				should_render_genre_texture = false;
		}

		// the game renders genre as UTF-16 in this version; widen our ASCII text
		genre_wide.assign(genre_string.begin(), genre_string.end());

		// Call the original, which will (or will not) in turn call our custom genre renderer.
		return original_fn(a1);
	}

	auto install_hook() -> void
	{
		// standard hook for getting the currently highlighted music entry
		MH_CreateHook(bm2dx::addr->MSELECT_GENRE_C, reinterpret_cast<LPVOID>(mselect_render_hook), &original_mselect_render_fn);

		// Swap the string the genre text call is about to draw. The sixth
		// argument still lives on the stack at the call, so it can be replaced
		// in place rather than by trampolining through the renderer.
		genre_text_hook = safetyhook::create_mid(bm2dx::addr->MSELECT_GENRE_A,
			[] (SafetyHookContext& ctx)
		{
			if (!genre_wide.empty())
				*reinterpret_cast<const wchar_t**>(ctx.rsp + text_argument_offset) = genre_wide.c_str();
		});

		// Answer the "does this entry have a genre texture?" check ourselves,
		// so a custom note-count line wins over the pre-rendered texture.
		genre_texture_hook = safetyhook::create_mid(bm2dx::addr->MSELECT_GENRE_B,
			[] (SafetyHookContext& ctx)
		{
			ctx.rip += texture_check_length;

			if (should_render_genre_texture)
				ctx.rflags &= ~flag_zero;
			else
				ctx.rflags |= flag_zero;
		});
	}
}
