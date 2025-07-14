#include <meta.h>
#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include "gui.h"
#include "log_window.h"
#include "main_window.h"
#include "../input.h"
#include "../hooks/renderer_hook.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace iidxtra::gui
{
	auto visible = false;
	auto input_enabled = true;
	auto play_lock_state = false;
    #if FORCE_EVENT_MODE_ENABLED == 0
    auto config_event_mode = false;
    #else
    auto config_event_mode = true;
    #endif

    auto wndproc_initialized = false;

    auto init() -> void
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        auto& io = ImGui::GetIO();

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

        // get backbuffer size from d3d9 device
        io.DisplaySize = ImVec2(1920, 1080);

        IDirect3DSurface9* back_buffer = nullptr;
        renderer_hook::device_ptr->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
        auto desc = D3DSURFACE_DESC {};
        if (FAILED(back_buffer->GetDesc(&desc)))
            back_buffer->Release();
        else
            io.DisplaySize = ImVec2(desc.Width, desc.Height);

        io.IniFilename = nullptr;
        io.BackendPlatformName = "imgui_impl_2dxtra";

        {
            auto& style = ImGui::GetStyle();
            style.FrameRounding = 0.0f;
			style.TabRounding = 0.f;
            style.WindowBorderSize = 0.0f;
			style.PopupBorderSize = 0.0f;

            auto colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_TextDisabled]           = ImVec4(0.73f, 0.75f, 0.74f, 1.00f);
            colors[ImGuiCol_WindowBg]               = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
            colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
            colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_FrameBg]                = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.84f, 0.66f, 0.66f, 0.40f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.84f, 0.66f, 0.66f, 0.67f);
            colors[ImGuiCol_TitleBg]                = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.47f, 0.22f, 0.22f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
            colors[ImGuiCol_MenuBarBg]              = ImVec4(0.34f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
            colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.71f, 0.39f, 0.39f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.84f, 0.66f, 0.66f, 1.00f);
            colors[ImGuiCol_Button]                 = ImVec4(0.47f, 0.22f, 0.22f, 0.65f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.71f, 0.39f, 0.39f, 0.65f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
            colors[ImGuiCol_Header]                 = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_HeaderHovered]          = ImVec4(0.84f, 0.66f, 0.66f, 0.65f);
            colors[ImGuiCol_HeaderActive]           = ImVec4(0.84f, 0.66f, 0.66f, 0.00f);
            colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
            colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_SeparatorActive]        = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_ResizeGrip]             = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
            colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
            colors[ImGuiCol_TabHovered]             = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
            colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
            colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
            colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        }

        ImGui_ImplDX9_Init(renderer_hook::device_ptr);
    }

    auto begin() -> void
    {
        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = visible;

        if (visible && input_enabled)
        {
        	io.AddKeyEvent(ImGuiKey_UpArrow, input::test_menu_button(1) || input::test_menu_button(8));
        	io.AddKeyEvent(ImGuiKey_DownArrow, input::test_menu_button(0) || input::test_menu_button(7));
        	io.AddKeyEvent(ImGuiKey_LeftArrow, input::test_turntable(false));
        	io.AddKeyEvent(ImGuiKey_RightArrow, input::test_turntable(true));
        	io.AddKeyEvent(ImGuiKey_Space, input::test_menu_button(5) || input::test_menu_button(12));
        }

		ImGui_ImplDX9_NewFrame();
		ImGui::NewFrame();
    }

    auto render() -> void
    {
		log_window::render();

		if (visible)
			main_window::render();
    }

    auto wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> void
    {
        if (!wndproc_initialized)
        {
            ImGui_ImplWin32_Init(hwnd);
            wndproc_initialized = true;
        }

        if (!visible)
            return;

        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    }

    auto end() -> void
    {
        ImGui::EndFrame();
        ImGui::Render();

        IDirect3DSurface9* backbuffer = nullptr;
        IDirect3DSurface9* render_target = nullptr;

        if (SUCCEEDED(renderer_hook::device_ptr->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)))
        {
            renderer_hook::device_ptr->GetRenderTarget(0, &render_target);
            renderer_hook::device_ptr->SetRenderTarget(0, backbuffer);
            backbuffer->Release();
        }

        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        renderer_hook::device_ptr->SetRenderTarget(0, render_target);
    }

    auto draw_hint_text(std::string_view text, float offset_y, ImU32 color) -> void
    {
        auto const screen_width = ImGui::GetIO().DisplaySize.x;
        auto const text_width = ImGui::CalcTextSize(text.data()).x;
        auto const base_y = (ImGui::GetIO().DisplaySize.y * 0.5f + ImGui::GetWindowSize().y * 0.5f + 15);

        ImGui::GetBackgroundDrawList()->AddText({(screen_width - text_width) * 0.5f, base_y + offset_y}, color, text.data());
    }
}