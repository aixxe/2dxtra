#pragma once

#include <d3d9.h>

namespace iidxtra::renderer_hook
{
    extern IDirect3DDevice9* device_ptr;
    extern IDirect3DSwapChain9* swapchain_ptr;

	auto install_hook() -> void;
	auto uninstall_hook() -> void;
}