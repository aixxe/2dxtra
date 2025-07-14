#include <mutex>
#include <MinHook.h>
#include "renderer_hook.h"
#include "../game.h"
#include "../gui/gui.h"
#include "../util/scoped_page_permissions.h"

namespace iidxtra::renderer_hook
{
    std::once_flag init_once;

    void** original_vft = nullptr;
    std::unique_ptr<std::uintptr_t[]> replacement_vft;

    IDirect3DDevice9* device_ptr = nullptr;
    IDirect3DSwapChain9* swapchain_ptr = nullptr;

    HRESULT (*original_present_fn) (IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD) = nullptr;
    LRESULT (*original_wndproc_fn) (void*, HWND, UINT, WPARAM, LPARAM) = nullptr;

    auto present_hook_fn(IDirect3DSwapChain9* swapchain, const RECT* src, const RECT* dst, HWND dst_wnd, const RGNDATA* dirty_region, DWORD flags) -> HRESULT
    {
        std::call_once(init_once, gui::init);

        if (SUCCEEDED(device_ptr->BeginScene()))
        {
            gui::begin();
            gui::render();
            gui::end();

            device_ptr->EndScene();
        }

        return original_present_fn(swapchain, src, dst, dst_wnd, dirty_region, flags);
    }

    auto wndproc_hook_fn(void* a1, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        auto result = original_wndproc_fn(a1, hwnd, msg, wparam, lparam);
        gui::wndproc(hwnd, msg, wparam, lparam);
        return result;
    }

	auto set_renderer_freeze(bool frozen) -> void
	{
		auto guard = util::scoped_page_permissions { bm2dx::addr->RENDERER_PATCH, 5, PAGE_EXECUTE_READWRITE };
    	auto static original = std::vector<std::uint8_t> { 0x00, 0x00, 0x00, 0x00, 0x00 };

    	if (frozen)
    	{
    		CopyMemory(original.data(), bm2dx::addr->RENDERER_PATCH, 5);
    		CopyMemory(bm2dx::addr->RENDERER_PATCH, "\x90\x90\x90\x90\x90", 5);
    	}
    	else
    	{
    		CopyMemory(bm2dx::addr->RENDERER_PATCH, original.data(), 5);
    	}

		Sleep(25);
	}

    auto install_hook() -> void
    {
		#ifndef NDEBUG
			set_renderer_freeze(true);
        #endif

        device_ptr = *reinterpret_cast<IDirect3DDevice9**>(bm2dx::addr->D3D9_DEVICE);

        if (device_ptr->GetSwapChain(0, &swapchain_ptr) != D3D_OK)
            return;

        original_vft = *reinterpret_cast<void***>(swapchain_ptr);
        auto count = 0;

        while (original_vft[count])
            ++count;

        replacement_vft = std::make_unique<std::uintptr_t[]>(count);
        std::memcpy(replacement_vft.get(), original_vft, count * sizeof(void*));

        replacement_vft[3] = std::uintptr_t(present_hook_fn);
        original_present_fn = reinterpret_cast<decltype(original_present_fn)>(original_vft[3]);

        *reinterpret_cast<void**>(swapchain_ptr) = replacement_vft.get();

        // Hook WndProc to capture keyboard/mouse input.
        MH_CreateHook(bm2dx::addr->WNDPROC_FN, wndproc_hook_fn, (void**) &original_wndproc_fn);
        MH_EnableHook(bm2dx::addr->WNDPROC_FN);

		#ifndef NDEBUG
			set_renderer_freeze(false);
		#endif
    }

    auto uninstall_hook() -> void
    {
		// Write the original virtual table pointer back.
		set_renderer_freeze(true);
        *reinterpret_cast<void**>(swapchain_ptr) = original_vft;
		set_renderer_freeze(false);
    }
}