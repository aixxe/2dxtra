#include <fmt/core.h>
#include <safetyhook.hpp>
#include "../game.h"
#include "../chart_set.h"
#include "result_title_hook.h"

namespace iidxtra::result_title_hook
{
    auto result_mid_fn_hook = safetyhook::MidHook {};

    using init_text_t = bm2dx::text_props_t* (*) (bm2dx::text_props_t*);
    using text_render_t = void (*) (int, int, int, int, bm2dx::text_props_t*, const char*);

	auto result_title_hook_fn(SafetyHookContext& ctx) -> void
    {
        auto static init_text = reinterpret_cast<init_text_t>(bm2dx::addr->TEXT_INIT_FN);
        auto static text_render = reinterpret_cast<text_render_t>(bm2dx::addr->TEXT_RENDER_FN);

        if (!chart_set::active.empty())
        {
            // the buffer isn't that big, so don't overflow it
            auto text = chart_set::active.substr(0, 64);

            // if we stripped characters, add an ellipsis
            if (text.size() != chart_set::active.size())
                text += "...";

            auto static properties = bm2dx::text_props_t {};
            init_text(&properties);
            properties.spacing = -1;
            properties.h_align = 1;
            properties.v_align = 1;

            auto string = fmt::format("<color {}>{}</color>", "70ff58ff", text.c_str());
            text_render(7, 960, 940, 0x84, &properties, string.c_str());
        }
	}

	auto install_hook() -> void
        { result_mid_fn_hook = safetyhook::create_mid(bm2dx::addr->RESULT_ARTIST_FN, result_title_hook_fn); }
}
