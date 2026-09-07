#include <algorithm>
#include <string_view>
#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../score_set.h"
#include "../util/code_patch.h"
#include "../hooks/chart_load_hook.h"
#include "../hooks/score_invalidator_hook.h"
#include "network_hook.h"

namespace iidxtra::network_hook
{
	auto blocked_requests = std::vector<std::string_view> {
		bm2dx::REQUEST_LOBBY_ENTRY,
		bm2dx::REQUEST_LOBBY_UPDATE,
		bm2dx::REQUEST_LOBBY_DELETE,
	};

	void* original_xrpc_apply_fn = nullptr;
	void* original_music_reg_fn = nullptr;
	void* original_dan_save_fn = nullptr;
    void* original_eappli_save_fn = nullptr;

	// todo: can't be used to situationally allow music.reg/nosave and potentially others, needs more testing
	auto xrpc_apply_hook_fn(void* handle, const char* method, void* shmem, void* cb, void* cbdata, void* valist) -> void*
	{
		auto const name = std::string_view { method };

		auto const blocked = std::ranges::any_of(blocked_requests,
			[&] (auto const& suffix) { return name.ends_with(suffix); });

		if (blocked)
		{
			log::debug("Blocked request '{}'", method);
			return nullptr;
		}

		// This always occurs after getRank, so the scores sent from the network should exist now.
        if (name.ends_with(bm2dx::REQUEST_GAME_SYSTEM_INFO))
			score_set::backup();

		return reinterpret_cast<void* (*) (void*, const char*, void*, void*, void*, void*)>(original_xrpc_apply_fn)(handle, method, shmem, cb, cbdata, valist);
	}

	// separate hook solely for dealing with score submissions
	auto music_reg_hook_fn() -> bool
	{
		// allow non-custom score submissions
		if (chart_load_hook::was_last_score_custom())
		{
            // invalidate the session -- dan course results shouldn't save for custom charts
            if (!score_invalidator_hook::is_session_invalid_p1)
                log::debug("P1 session invalidated");

            if (!score_invalidator_hook::is_session_invalid_p2)
                log::debug("P2 session invalidated");

            score_invalidator_hook::is_session_invalid_p1 = true;
            score_invalidator_hook::is_session_invalid_p2 = true;

			auto static patch = util::code_patch { bm2dx::addr->REG_PATCH_ADDR,
				{ 0x90, 0x90, 0x90, 0x90, 0x90 } };

			patch.enable();
			auto result = reinterpret_cast<bool (*) ()>(original_music_reg_fn)();
			patch.disable();

			// done!
			return result;
		}

		return reinterpret_cast<bool (*) ()>(original_music_reg_fn)();
	}

    // separate hook solely for dealing with dan course result submissions
	auto dan_save_hook_fn(int player, int a2, int a3, int a4, int a5, void* a6, int a7, char a8, char a9, char a10) -> void*
	{
        if (player == 0 && score_invalidator_hook::is_session_invalid_p1)
            return nullptr;

        if (player == 1 && score_invalidator_hook::is_session_invalid_p2)
            return nullptr;

		return reinterpret_cast<void* (*) (int, int, int, int, int, void*, int, char, char, char)>(original_dan_save_fn)
            (player, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	}

    // determines whether eappliresult should be sent
    auto eappli_save_hook_fn(int player, void* a2) -> void*
    {
		// prevent cheat modifier scores to be saved
		// no_save would be set to 1 anyway, but this feels justified
        if (player == 0 && score_invalidator_hook::is_play_invalid_p1)
            return nullptr;

        if (player == 1 && score_invalidator_hook::is_play_invalid_p2)
            return nullptr;

        // don't allow custom chart scores to be saved
        if (chart_load_hook::was_last_score_custom())
            return nullptr;

        return reinterpret_cast<void* (*) (int, void*)>(original_eappli_save_fn)(player, a2);
    }

	auto install_hook() -> void
	{
		MH_CreateHook(bm2dx::addr->XRPC_APPLY_FN, reinterpret_cast<LPVOID>(xrpc_apply_hook_fn), &original_xrpc_apply_fn);
		MH_CreateHook(bm2dx::addr->REG_DISPATCH_FN, reinterpret_cast<LPVOID>(music_reg_hook_fn), &original_music_reg_fn);
		MH_CreateHook(bm2dx::addr->DAN_SAVE_FN, reinterpret_cast<LPVOID>(dan_save_hook_fn), &original_dan_save_fn);
        MH_CreateHook(bm2dx::addr->EAAPPLI_SAVE_FN, reinterpret_cast<LPVOID>(eappli_save_hook_fn), &original_eappli_save_fn);

		// Force arena phase to 1. (local only)
		auto static arena_patch = util::code_patch { bm2dx::addr->ARENA_PHASE_PATCH,
			{ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 } }; // mov eax, 1; ret

		arena_patch.enable();
	}
}