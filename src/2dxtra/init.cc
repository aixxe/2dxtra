#include <MinHook.h>
#include "chart_set.h"
#include "score_set.h"
#include "hooks/input_hook.h"
#include "hooks/network_hook.h"
#include "hooks/chart_load_hook.h"
#include "hooks/dan_select_hook.h"
#include "hooks/music_select_hook.h"
#include "hooks/stage_result_hook.h"
#include "hooks/mselect_genre_hook.h"
#include "hooks/mdata_load_hook.h"
#include "hooks/renderer_hook.h"
#include "hooks/reset_state_hook.h"
#include "hooks/result_title_hook.h"
#include "hooks/score_invalidator_hook.h"
#include "hooks/card_out_hook.h"
#include "hooks/play_field_load_hook.h"
#include "hooks/judge_timing_hook.h"
#include "hooks/attract_randomizer_hook.h"
#include "features/autoplay.h"
#include "features/unrandomizer.h"
#include "features/autoretry.h"

namespace iidxtra
{
	auto init(LPVOID param) -> DWORD
	{
		auto const module = static_cast<HMODULE>(param);

		MH_Initialize();

		// Find custom charts on disk.
		wchar_t module_path[MAX_PATH] = {};
		GetModuleFileNameW(module, module_path, MAX_PATH);

		auto const cwd = std::filesystem::path { module_path }.remove_filename();
		mdata_load_hook::gather_custom_sets(cwd/"2dxtra/charts");

		// Allocate memory.
		score_set::init();

		// Initialize hooks.
		input_hook::install_hook();
		network_hook::install_hook();
		chart_load_hook::install_hook();
		dan_select_hook::install_hook();
		music_select_hook::install_hook();
		stage_result_hook::install_hook();
		mselect_genre_hook::install_hook();
		mdata_load_hook::install_hook();
        reset_state_hook::install_hook();
        result_title_hook::install_hook();
		score_invalidator_hook::install_hook();
        card_out_hook::install_hook();
        autoretry::install_hook();
        play_field_load_hook::install_hook();
        judge_timing_hook::install_hook();
		autoplay::install_hook();
		unrandomizer::install_hook();
		attract_randomizer_hook::install_hook();

		// Enable all hooks.
		MH_EnableHook(MH_ALL_HOOKS);

#ifndef NDEBUG
		// Wait for detach signal.
		while (true)
		{
			if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_F10))
			{
				// Revert to default before unloading.
				chart_set::revert();
				break;
			}

			Sleep(10);
		}

		// Free stock score storage.
		score_set::uninit();

		// Hooks that require extra setup.
		mselect_genre_hook::uninstall_hook();
        renderer_hook::uninstall_hook();
		card_out_hook::uninstall_hook();

		MH_Uninitialize();
		FreeLibraryAndExitThread(static_cast<HMODULE>(param), 0);
#endif

		return 0;
	}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (!bm2dx::resolve())
			return FALSE;

		bm2dx::set_soft_rev('E');

		CreateThread(nullptr, 0, iidxtra::init, module, 0, nullptr);
	}

	return TRUE;
}