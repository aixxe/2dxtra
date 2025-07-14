#include "log.h"
#include <ranges>

namespace iidxtra::log
{
	auto base_framerate = std::uint32_t { 60 };
	auto max_display_time = std::uint32_t { 4 };

	std::vector<std::pair<std::uint32_t, std::string>> lines {};

	auto opacity() -> float
	{
		if (lines.empty())
			return 0.f;

		auto max = std::uint32_t {};

		for (auto const& time: lines | std::views::keys)
			if (time > max)
				max = time;

		return max < base_framerate ? static_cast<float>(max) / static_cast<float>(base_framerate): 1.f;
	}

	auto empty() -> bool
		{ return lines.empty(); }
}
