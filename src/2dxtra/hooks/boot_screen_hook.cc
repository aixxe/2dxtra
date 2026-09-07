#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <safetyhook.hpp>

#include "../game.h"
#include "boot_screen_hook.h"

namespace iidxtra::boot_screen_hook
{
    namespace
    {
        auto constexpr RAINBOW_SPEED = 0.50f;
        auto constexpr RAINBOW_SPACING = 0.030f;
        auto constexpr RAINBOW_SATURATION = 0.60f;
        auto constexpr BOOT_MODEL_ARGUMENT_OFFSET = 0x20;
        auto constexpr TITLE_MODEL_ARGUMENT_OFFSET = 0x28;

        safetyhook::MidHook model_text_hook {};
        safetyhook::MidHook title_model_text_hook {};
        std::string colored_model {};

        auto append_hex_byte(std::string& output, std::uint8_t value) -> void
        {
            auto constexpr hex = "0123456789abcdef";
            output.push_back(hex[value >> 4]);
            output.push_back(hex[value & 0x0f]);
        }

        auto hue_to_rgb(float hue) -> std::array<std::uint8_t, 3>
        {
            hue -= std::floor(hue);
            auto const scaled = hue * 6.0f;
            auto const sector = static_cast<int>(scaled);
            auto const fraction = scaled - static_cast<float>(sector);
            auto const descending = static_cast<std::uint8_t>((1.0f - fraction) * 255.0f);
            auto const ascending = static_cast<std::uint8_t>(fraction * 255.0f);
            auto const brighten = [] (std::uint8_t channel) -> std::uint8_t
            {
                return static_cast<std::uint8_t>(
                    255.0f - (255.0f - static_cast<float>(channel)) * RAINBOW_SATURATION);
            };

            std::array<std::uint8_t, 3> rgb {};

            switch (sector)
            {
                case 0: rgb = { 255, ascending, 0 }; break;
                case 1: rgb = { descending, 255, 0 }; break;
                case 2: rgb = { 0, 255, ascending }; break;
                case 3: rgb = { 0, descending, 255 }; break;
                case 4: rgb = { ascending, 0, 255 }; break;
                default: rgb = { 255, 0, descending }; break;
            }

            return { brighten(rgb[0]), brighten(rgb[1]), brighten(rgb[2]) };
        }

        auto make_rainbow_text(const char* model) -> const char*
        {
            using clock = std::chrono::steady_clock;
            auto static const animation_start = clock::now();
            auto const elapsed = std::chrono::duration<float>(clock::now() - animation_start).count();
            auto const phase = elapsed * RAINBOW_SPEED;

            colored_model.clear();

            for (std::size_t i = 0; model[i] != '\0'; ++i)
            {
                colored_model.append("<color ");
                auto const color = hue_to_rgb(phase + static_cast<float>(i) * RAINBOW_SPACING);
                append_hex_byte(colored_model, color[0]);
                append_hex_byte(colored_model, color[1]);
                append_hex_byte(colored_model, color[2]);
                colored_model.append("ff>");
                colored_model.push_back(model[i]);
                colored_model.append("</color>");
            }

            return colored_model.c_str();
        }

        auto recolor_model_argument(SafetyHookContext& ctx, std::uintptr_t argument_offset) -> void
        {
            auto** const text_argument = reinterpret_cast<const char**>(ctx.rsp + argument_offset);
            auto const model = *text_argument;

            if (!model || model != reinterpret_cast<const char*>(bm2dx::addr->GAME_MODEL))
                return;

            *text_argument = make_rainbow_text(model);
        }

        auto model_text_call_hook(SafetyHookContext& ctx) -> void
            { recolor_model_argument(ctx, BOOT_MODEL_ARGUMENT_OFFSET); }

        auto title_model_text_call_hook(SafetyHookContext& ctx) -> void
            { recolor_model_argument(ctx, TITLE_MODEL_ARGUMENT_OFFSET); }
    }

    auto install_hook() -> void
    {
        model_text_hook = safetyhook::create_mid(
            bm2dx::addr->BOOT_MODEL_TEXT_CALL, model_text_call_hook);
        title_model_text_hook = safetyhook::create_mid(
            bm2dx::addr->TITLE_MODEL_TEXT_CALL, title_model_text_call_hook);
    }
}
