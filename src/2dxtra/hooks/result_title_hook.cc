#include <fmt/format.h>
#include <safetyhook.hpp>
#include "../game.h"
#include "../chart_set.h"
#include "../features/chart_speed.h"
#include "result_title_hook.h"

namespace iidxtra::result_title_hook
{
    auto result_mid_fn_hook = safetyhook::MidHook {};

    using init_text_t = bm2dx::text_props_t* (*) (bm2dx::text_props_t*);
    using text_render_t = void (*) (int, int, int, int, bm2dx::text_props_t*, const char*);

    // Where the extra line goes on the result screen, and how much of it fits.
    auto constexpr text_font = 7;
    auto constexpr text_x = 960;
    auto constexpr text_y = 930;
    auto constexpr text_layer = 0x84;
    auto constexpr text_max_length = 64;

	auto result_title_hook_fn(SafetyHookContext& ctx) -> void
    {
        auto static init_text = reinterpret_cast<init_text_t>(bm2dx::addr->TEXT_INIT_FN);
        auto static text_render = reinterpret_cast<text_render_t>(bm2dx::addr->TEXT_RENDER_FN);

        auto text = chart_set::active;

        if (chart_speed::rate != 1.0f)
            text += fmt::format("{}x{:.2f}", text.empty() ? "" : " ", chart_speed::rate);

        if (text.empty())
            return;

        // the buffer isn't that big, so don't overflow it
        auto const full_text = text;
        text = text.substr(0, text_max_length);

        // if we stripped characters, add an ellipsis
        if (text.size() != full_text.size())
            text += "...";

        auto static properties = bm2dx::text_props_t {};
        init_text(&properties);
        properties.spacing = -1;
        properties.h_align = 1;
        properties.v_align = 1;

        auto string = fmt::format("<color {}>{}</color>", "70ff58ff", text.c_str());
        text_render(text_font, text_x, text_y, text_layer, &properties, string.c_str());
	}

	auto install_hook() -> void
        { result_mid_fn_hook = safetyhook::create_mid(bm2dx::addr->RESULT_ARTIST_FN, result_title_hook_fn); }
}
