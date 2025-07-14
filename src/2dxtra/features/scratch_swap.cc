#include <random>
#include "../log.h"
#include "../hooks/score_invalidator_hook.h"
#include "scratch_swap.h"

namespace iidxtra::scratch_swap
{
	auto enabled_p1 = false;
	auto enabled_p2 = false;

	auto swap_lane_p1 = std::int8_t { -1 };
	auto swap_lane_p2 = std::int8_t { -1 };

    void reset()
    {
        enabled_p1 = false;
        enabled_p2 = false;
        swap_lane_p1 = -1;
        swap_lane_p2 = -1;
    }

	auto mutate(const std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void
	{
    	auto rng = std::default_random_engine { std::random_device {} () };
		auto dist = std::uniform_int_distribution { 0, 6 };

		auto const enabled = (player == 0 ? enabled_p1: enabled_p2);

		if (!enabled)
		{
			if (bm2dx::state->play_style == 0)
				return;

			if (bm2dx::state->play_style == 1 && (!enabled_p1 && !enabled_p2))
				return;
		}

		auto const target_lane_p1 = (swap_lane_p1 == -1 ? dist(rng): swap_lane_p1);
		auto const target_lane_p2 = (swap_lane_p2 == -1 ? dist(rng): swap_lane_p2);
		auto const target_lane = (player == 0 ? target_lane_p1: target_lane_p2);

		for (auto& event: buffer)
		{
			if (event.type == 6)
				break;

			if (bm2dx::state->play_style == 1)
			{
				if (enabled_p1 && (event.type == 0 || event.type == 2))
				{
					// LEFT play side (use P1 lane setting)
					if (event.parameter == target_lane_p1)
						event.parameter = 7;
					else if (event.parameter == 7)
						event.parameter = target_lane_p1;
				}
				else if (enabled_p2 && (event.type == 1 || event.type == 3))
				{
					// RIGHT play side (use P2 lane setting)
					if (event.parameter == target_lane_p2)
						event.parameter = 7;
					else if (event.parameter == 7)
						event.parameter = target_lane_p2;
				}
			}
			else
			{
				if (event.type != 0 && event.type != 2)
					continue;

				if (event.parameter == target_lane)
					event.parameter = 7;
				else if (event.parameter == 7)
					event.parameter = target_lane;
			}
		}

		if (bm2dx::state->play_style == 1)
		{
			if (enabled_p1) log::print("[Un-randomizer] Swapped lane {} to scratch (LEFT)", target_lane_p1 + 1);
			if (enabled_p2) log::print("[Un-randomizer] Swapped lane {} to scratch (RIGHT)", target_lane_p2 + 1);
			score_invalidator_hook::invalidate(bm2dx::state->p1_active ? 0: 1);
		}
		else
		{
			log::print("[Un-randomizer] P{}: Swapped lane {} to scratch", player + 1, target_lane + 1);
			score_invalidator_hook::invalidate(player);
		}
	}
}