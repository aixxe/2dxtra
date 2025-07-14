#pragma once

namespace iidxtra::score_invalidator_hook
{
	extern bool is_play_invalid_p1, is_play_invalid_p2, is_session_invalid_p1, is_session_invalid_p2;

	auto invalidate(std::uint8_t player) -> void;
	auto reset(std::uint8_t player) -> void;
    auto reset_session() -> void;

	auto install_hook() -> void;
}