#include <MinHook.h>
#include <fmt/core.h>
#include "../game.h"
#include "../input.h"
#include "../chart_set.h"
#include "../util/memory.h"
#include "../util/scoped_page_permissions.h"
#include "mselect_genre_hook.h"

namespace iidxtra::mselect_genre_hook
{
	void* original_mselect_render_fn = nullptr;
	std::uint8_t original_genre_bytes[5] = {};
	std::uint8_t original_texture_check_bytes[7] = {};

	bm2dx::music_entry_t* music = nullptr;
	bm2dx::music_entry_t* last_music = nullptr;
	std::uint32_t should_render_genre_texture = 0;

	std::string genre_string = {};

	/**
	 * Utility function for generating the formatted genre text string.
	 */
	auto append_genre_string(std::string& text, std::unordered_map<std::uint8_t, chart_set::chart_t>& original_charts,
		std::unordered_map<std::uint8_t, chart_set::chart_t>& custom_charts, std::uint32_t chart_id,
		char prefix, const char* color, bool show_delta) -> void
	{
		if (custom_charts.contains(chart_id))
		{
			auto const original = original_charts.at(chart_id).notes;
			auto const custom = custom_charts.at(chart_id).notes;

			auto delta = static_cast<std::int32_t>(custom - original);
			auto delta_str = (delta > 0 ? "+": "");

			if (!show_delta) {
				delta_str = "=";
				delta = custom;
			}

			if (custom != original)
                text.append(fmt::format("{}<color {}>{}{}</color> ", prefix, color, delta_str, delta));
		}
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
			auto show_delta = !input::test_game_button(18);

			if (active_set.music.contains(music->id))
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

		// Call the original, which will (or will not) in turn call our custom genre renderer.
		return original_fn(a1);
	}

	/**
	 * Mid-function hook used to replace genre text with our custom text.
	 */
	auto mselect_genre_hook(int a1, int a2, signed int a3, void* a4, void* a5, const char* a6) -> void
	{
		using hook_fn_t = void (*) (int, int, signed int, void*, void*, const char*);
		auto static original_fn = reinterpret_cast<hook_fn_t>(bm2dx::addr->TEXT_RENDER_FN);
		original_fn(a1, a2, a3, a4, a5, !genre_string.empty() ? genre_string.c_str(): a6);
	}

	// todo: LV.100 mega cursed ancient code in dire need of simplification w/SafetyHookMid
	auto install_hook() -> void
	{
		// standard hook for getting the currently highlighted music entry
		MH_CreateHook(bm2dx::addr->MSELECT_GENRE_C, mselect_render_hook, &original_mselect_render_fn);

		{
			std::uint8_t detour[] =
			{
				0xFF, 0x15, 0x02, 0x00, 0x00, 0x00, 0xEB, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0xE9, 0x00, 0x00, 0x00, 0x00
			};

			// allocate some memory nearby for a mid-function hook
			auto buffer = util::alloc_near_address(bm2dx::addr->MSELECT_GENRE_A);

			// absolute jump to our hook function
			*reinterpret_cast<std::uint64_t*>(detour + 8) = std::uint64_t(mselect_genre_hook);

			{
				// relative jump back to the game code
				auto rip = std::uintptr_t(buffer) + 16;
				auto offset = std::int32_t(std::uintptr_t(bm2dx::addr->MSELECT_GENRE_A + 5) - rip) - 5;

				*reinterpret_cast<std::uint32_t*>(detour + 17) = std::uint32_t(offset);
			}

			// write detour code
			CopyMemory(buffer, detour, sizeof(detour));

			// get displacement to our detour function
			auto rip = std::uintptr_t(bm2dx::addr->MSELECT_GENRE_A);
			auto offset = std::int32_t(std::uintptr_t(buffer) - rip) - 5;

			std::uint8_t patch_bytes[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
			*reinterpret_cast<std::int32_t*>(patch_bytes + 1) = offset;

			// patch game code to jump to our detour
			auto guard = util::scoped_page_permissions { bm2dx::addr->MSELECT_GENRE_A, sizeof(patch_bytes), PAGE_EXECUTE_READWRITE };

			CopyMemory(original_genre_bytes, bm2dx::addr->MSELECT_GENRE_A, sizeof(patch_bytes));
			CopyMemory(bm2dx::addr->MSELECT_GENRE_A, patch_bytes, sizeof(patch_bytes));
		}

		{
			std::uint8_t detour[] = {
				0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // move address of texture flag into rcx
				0x83, 0x39, 0x00,                                           // compare value at rcx
				0xE9, 0x00, 0x00, 0x00, 0x00                                // jump back to game
			};

			// Insert address of the 'should-render-genre-texture' boolean.
			*reinterpret_cast<std::uint64_t*>(detour + 2) = std::uint64_t(&should_render_genre_texture);

			// Allocate memory & write the detour code.
			auto buffer = util::alloc_near_address(bm2dx::addr->MSELECT_GENRE_B);

			{
				// Calculate address to jump back to the game code.
				auto rip = std::uintptr_t(buffer) + 13;
				auto offset = std::int32_t(std::uintptr_t(bm2dx::addr->MSELECT_GENRE_B + 7) - rip) - 5;

				*reinterpret_cast<std::uint32_t*>(detour + 14) = std::uint32_t(offset);
			}

			// Ready to write the detour code now.
			CopyMemory(buffer, detour, sizeof(detour));

			// Define the bytes that will be overwriting the game instructions.
			// Insert the relative jump to the detour code in the placeholder bytes again.
			// The bytes after the call are used to check the return value.
			// Then we utilize the existing jump-if-zero to either use the custom genre texture or our text.
			std::uint8_t patch_bytes[] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90 };

			{
				auto rip = std::uintptr_t(bm2dx::addr->MSELECT_GENRE_B);
				auto offset = std::int32_t(std::uintptr_t(buffer) - rip) - 5;

				*reinterpret_cast<std::int32_t*>(patch_bytes + 1) = offset;
			}

			// Update page permissions so we can write code.
			auto guard = util::scoped_page_permissions { bm2dx::addr->MSELECT_GENRE_B, sizeof(patch_bytes), PAGE_EXECUTE_READWRITE };

			// Backup the original bytes so we can detach without crashing in debug builds.
			CopyMemory(original_texture_check_bytes, bm2dx::addr->MSELECT_GENRE_B, sizeof(patch_bytes));

			// Overwrite the game code.
			CopyMemory(bm2dx::addr->MSELECT_GENRE_B, patch_bytes, sizeof(patch_bytes));
		}
	}

	auto uninstall_hook() -> void
	{
		{
			auto guard = util::scoped_page_permissions { bm2dx::addr->MSELECT_GENRE_A, sizeof(original_genre_bytes), PAGE_EXECUTE_READWRITE };
			CopyMemory(bm2dx::addr->MSELECT_GENRE_A, original_genre_bytes, sizeof(original_genre_bytes));
		}

		{
			auto guard = util::scoped_page_permissions { bm2dx::addr->MSELECT_GENRE_B, sizeof(original_texture_check_bytes), PAGE_EXECUTE_READWRITE };
			CopyMemory(bm2dx::addr->MSELECT_GENRE_B, original_texture_check_bytes, sizeof(original_texture_check_bytes));
		}
	}
}