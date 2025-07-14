#include "regular_speed.h"
#include "../hooks/score_invalidator_hook.h"

namespace iidxtra::regular_speed
{
	auto enabled = false;

    auto reset() -> void
		{ enabled = false; }

	auto mutate(const std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void
	{
		if (!enabled)
			return;

		auto events = std::vector<bm2dx::chart_event_t>();
		events.reserve(0x3000);

		auto skipped = 0;
		auto saw_first = false;

		for (auto const& event: buffer)
		{
			if (event.type == 4)
			{
				if (!saw_first)
				{
					// need to copy the first one which sets the initial BPM...
					saw_first = true;
				}
				else
				{
					// ...but ignore any subsequent tempo events.
					skipped++;
					continue;
				}
			}

			events.emplace_back(event);

			if (event.type == 6)
				break;
		}

		if (skipped == 0)
			return;

		score_invalidator_hook::invalidate(player);

		buffer = events;
	}
}