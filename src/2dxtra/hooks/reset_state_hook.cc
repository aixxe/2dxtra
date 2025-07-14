#include <meta.h>
#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "../gui/gui.h"
#include "../chart_set.h"
#include "../score_set.h"
#include "../features/autoplay.h"
#include "../features/cn_override.h"
#include "../features/regular_speed.h"
#include "../features/scratch_swap.h"
#include "../features/unrandomizer.h"
#include "../features/keysound_switch.h"
#include "../features/cn_transformer.h"
#include "../features/play_visuals.h"
#include "../features/timing_modifier.h"
#include "../features/autoretry.h"
#include "reset_state_hook.h"
#include "score_invalidator_hook.h"

namespace iidxtra::reset_state_hook
{
	void* original_reset_state_fn = nullptr;

	auto reset_state_hook_fn(std::uint32_t a1) -> void*
	{
        // revert to default charts
        chart_set::revert();

        // mark session as valid
        score_invalidator_hook::reset_session();

		// clear scores
		score_set::custom.clear();

		if (score_set::stock_p1 != nullptr && score_set::stock_p2 != nullptr)
		{
			ZeroMemory(score_set::stock_p1, sizeof(bm2dx::player_scores_t));
			ZeroMemory(score_set::stock_p2, sizeof(bm2dx::player_scores_t));
		}

		log::debug("Cleared score data");

        // reset configuration
        if (gui::config_event_mode)
        {
            log::debug("Configuration reset");

			gui::play_lock_state = false;

            autoplay::reset();
            cn_override::reset();
            regular_speed::reset();
            scratch_swap::reset();
            unrandomizer::reset();
            keysound_switch::reset();
            cn_transformer::reset();
            play_visuals::reset();
            timing_modifier::reset();
            autoretry::reset();
        }

		return reinterpret_cast<void* (*) (std::uint32_t)>(original_reset_state_fn)(a1);
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->RESET_STATE_FN, reset_state_hook_fn, &original_reset_state_fn); }
}