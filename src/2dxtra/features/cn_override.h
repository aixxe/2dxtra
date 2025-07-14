#pragma once

namespace iidxtra::cn_override
{
	extern int p1_override_type;
	extern int p2_override_type;

    auto reset() -> void;

	auto set_initial_states(std::uint8_t player) -> void;
	auto update(std::uint8_t player) -> void;
}