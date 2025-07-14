#pragma once

#include <imgui.h>
#include <windows.h>
#include <string_view>

class IDirect3DDevice9;

namespace iidxtra::gui
{
	extern bool visible;
	extern bool input_enabled;
	extern bool play_lock_state;
    extern bool config_event_mode;

    auto init() -> void;
    auto begin() -> void;
    auto render() -> void;
    auto wndproc(HWND, UINT, WPARAM, LPARAM) -> void;
    auto end() -> void;

    auto draw_hint_text(std::string_view text, float offset_y, ImU32 color) -> void;
}