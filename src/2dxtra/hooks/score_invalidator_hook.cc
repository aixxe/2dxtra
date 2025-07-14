#include <MinHook.h>
#include "../log.h"
#include "../game.h"
#include "score_invalidator_hook.h"

namespace iidxtra::score_invalidator_hook
{
	// whether scores should be saved for this play
	bool is_play_invalid_p1 = false;
	bool is_play_invalid_p2 = false;

    // whether any score in this session was invalidated
    // this is used to determine whether to save course results
    bool is_session_invalid_p1 = false;
    bool is_session_invalid_p2 = false;

	void* original_score_invalidator_fn = nullptr;

    auto score_invalidator_hook_fn(std::uint32_t player, char a2) -> bool
    {
		// fail early if this score is already invalid
        if (reinterpret_cast<bool (*) (std::uint32_t, char)>(original_score_invalidator_fn)(player, a2))
			return true;

		// invalidate the score if illegal 2dxtra modifiers were used
		if ((player == 0 && is_play_invalid_p1) || (player == 1 && is_play_invalid_p2))
			return true;

		// score is valid!
		return false;
	}

	auto invalidate(std::uint8_t player) -> void
	{
		if (player == 0)
		{
		    if (!is_play_invalid_p1)
                log::debug("P1 score invalidated");

            if (!is_session_invalid_p1)
                log::debug("P1 session invalidated");

            is_play_invalid_p1 = true;
            is_session_invalid_p1 = true;
        }
		else if (player == 1)
		{
		    if (!is_play_invalid_p2)
                log::debug("P2 score invalidated");

            if (!is_session_invalid_p2)
                log::debug("P2 session invalidated");

            is_play_invalid_p2 = true;
            is_session_invalid_p2 = true;
        }
	}

	auto reset(std::uint8_t player) -> void
	{
		log::debug("P{} score is valid again", player + 1);

		if (player == 0)
			is_play_invalid_p1 = false;
		else if (player == 1)
			is_play_invalid_p2 = false;
	}

    auto reset_session() -> void
    {
        is_session_invalid_p1 = false;
        is_session_invalid_p2 = false;
    }

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->SCORE_INVALID_FN, score_invalidator_hook_fn, &original_score_invalidator_fn); }
}