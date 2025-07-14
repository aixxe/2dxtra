#pragma once

#include <vector>
#include <fmt/core.h>

namespace iidxtra::log
{
	extern std::uint32_t base_framerate;
	extern std::uint32_t max_display_time;

	extern std::vector<std::pair<std::uint32_t, std::string>> lines;

	template <typename... Args> void constexpr init(fmt::format_string<Args...> format, Args&&... args)
		{ lines.push_back(std::make_pair(base_framerate * 8, "INIT: " + fmt::format(format, std::forward<Args>(args)...))); }

	template <typename... Args> void constexpr print(fmt::format_string<Args...> format, Args&&... args)
		{ lines.push_back(std::make_pair(base_framerate * max_display_time, "INFO: " + fmt::format(format, std::forward<Args>(args)...))); }

	template <typename... Args> void constexpr debug(fmt::format_string<Args...> format, Args&&... args)
	{
#ifndef NDEBUG
		lines.push_back(std::make_pair(base_framerate * max_display_time, "DEBUG: " + fmt::format(format, std::forward<Args>(args)...)));
#endif
	}

	auto opacity() -> float;
	auto empty() -> bool;
}