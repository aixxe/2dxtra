#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../score_set.h"
#include "../util/scoped_page_permissions.h"
#include "../hooks/chart_load_hook.h"
#include "../hooks/score_invalidator_hook.h"
#include "network_hook.h"

namespace iidxtra::network_hook
{
	auto blocked_requests = std::vector<std::string> {
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
		if (std::ranges::find(blocked_requests, method) != blocked_requests.end())
		{
			log::debug("Blocked request '{}'", method);
			return nullptr;
		}

		// This always occurs after getRank, so the scores sent from the network should exist now.
        if (std::string_view(method) == bm2dx::REQUEST_GAME_SYSTEM_INFO)
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

			// backup the bytes that would make the network request
			std::uint8_t original_bytes[5] = {};
			CopyMemory(original_bytes, bm2dx::addr->REG_PATCH_ADDR, sizeof(original_bytes));

			// overwrite them with nops
            // todo: replace this abomination with mid-fn safetyhook
			{
	            auto guard = util::scoped_page_permissions { bm2dx::addr->REG_PATCH_ADDR, 5, PAGE_EXECUTE_READWRITE };
				CopyMemory(bm2dx::addr->REG_PATCH_ADDR, "\x90\x90\x90\x90\x90", 5);
				FlushInstructionCache(GetCurrentProcess(), bm2dx::addr->REG_PATCH_ADDR, 5);
			}

			// call the original function
			auto result = reinterpret_cast<bool (*) ()>(original_music_reg_fn)();

			// restore the original code
			{
				auto guard = util::scoped_page_permissions { bm2dx::addr->REG_PATCH_ADDR, 5, PAGE_EXECUTE_READWRITE };
				CopyMemory(bm2dx::addr->REG_PATCH_ADDR, original_bytes, sizeof(original_bytes));
				FlushInstructionCache(GetCurrentProcess(), bm2dx::addr->REG_PATCH_ADDR, 5);
			}

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
		MH_CreateHook(bm2dx::addr->XRPC_APPLY_FN, xrpc_apply_hook_fn, &original_xrpc_apply_fn);
		MH_CreateHook(bm2dx::addr->REG_DISPATCH_FN, music_reg_hook_fn, &original_music_reg_fn);
		MH_CreateHook(bm2dx::addr->DAN_SAVE_FN, dan_save_hook_fn, &original_dan_save_fn);
        MH_CreateHook(bm2dx::addr->EAAPPLI_SAVE_FN, eappli_save_hook_fn, &original_eappli_save_fn);

		// Force arena phase to 2. (local only)
		{
            auto guard = util::scoped_page_permissions { bm2dx::addr->ARENA_PHASE_PATCH, 6, PAGE_EXECUTE_READWRITE };
			CopyMemory(bm2dx::addr->ARENA_PHASE_PATCH, "\xB8\x01\x00\x00\x00\xC3", 6);
		}
	}
}