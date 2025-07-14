#pragma once

#include "game.h"
#include <filesystem>
#include <unordered_map>

namespace iidxtra::chart_set
{
	struct chart_t
	{
		std::string id;
		std::filesystem::path path;
		std::uint32_t notes;
		bm2dx::notes_radar_t radar;
	};

	struct music_t
	{
		std::unordered_map<std::uint8_t, chart_t> charts;
	};

	struct chart_set_t
	{
		std::unordered_map<std::uint32_t, music_t> music;
		std::size_t count; // total amount of charts
	};

	// whether we can currently switch chart sets
	// enabled in music select, disabled during gameplay
	extern bool switch_enabled;

	// minimal representation of music_data.bin
	extern chart_set_t stock;

	// discovered custom chart sets
	extern std::unordered_map<std::string, chart_set_t> custom;

	// currently active chart set
	extern std::string active;

	auto revert() -> void;
	auto set_active(const std::string& name) -> void;
}