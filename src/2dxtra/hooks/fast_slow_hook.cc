#include <MinHook.h>
#include <fmt/format.h>
#include <cstring>
#include <mutex>
#include <string_view>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#include "../game.h"
#include "../judgment.h"
#include "../features/fast_slow_display.h"
#include "fast_slow_hook.h"

namespace iidxtra::fast_slow_hook
{
    // Replace FAST/SLOW sprites with millisecond text rendered by us:
    //
    // 1. judge_apply_hook_fn captures the millisecond timing value.
    // 2. judge_display_hook_fn updates our stored timing to match the game's judgment.
    // 3. judge_draw_fs_keys_hook_fn & judge_draw_fs_sc_hook_fn call draw_indicator
    //    to prepare the timing text, then call the game's original draw function.
    // 4. sprite_draw_hook_fn receives the sprite's x/y coordinates, hides the
    //    FAST/SLOW sprite, and draws our text in its place.

    using fast_slow_display::polarity;
    using fast_slow_display::timing_t;

    // Native candidate record; the judgment context starts with eight records per player.
    struct judge_apply_hook_context_t
    {
        // timing value in milliseconds
        float milliseconds;
        std::byte reserved_04[4];
        // pointer to in-game note; used to check if judgement is valid (has note)
        void* note;
    };
    static_assert(sizeof(judge_apply_hook_context_t) == 16);
    static_assert(offsetof(judge_apply_hook_context_t, note) == 8);

    // context object passed to judge_display_hook_fn.
    struct alignas(8) judge_display_hook_context_t
    {
        std::byte reserved_00[8];

        // p1 (0) or p2 (1)
        int player;

        std::byte reserved_0c[20];

        // code:
        //   4 = PGREAT
        //   3 = FAST
        //   5 = SLOW
        //   8 = MISS
        int combined_code;

        std::byte reserved_24[12];
        int key_code;
        std::byte reserved_34[4];
        int scratch_code;
        std::byte reserved_3c[4];
    };
    static_assert(sizeof(judge_display_hook_context_t) == 64);
    static_assert(offsetof(judge_display_hook_context_t, player) == 8);
    static_assert(offsetof(judge_display_hook_context_t, combined_code) == 32);
    static_assert(offsetof(judge_display_hook_context_t, key_code) == 48);
    static_assert(offsetof(judge_display_hook_context_t, scratch_code) == 56);

    // Native function signatures and original functions populated by MinHook.
    using judge_apply_t = int (*)(void* context, int player, int grade, int lane, int score_index);
    using judge_display_t = std::intptr_t (*)(void* display, int code, int combo, bool scratch);
    using draw_indicator_t = void (*)(void* display);
    using judge_display_init_t = std::intptr_t (*)(void* display, int player, int mode);
    using sprite_draw_t = void* (*)(void* manager, const char* name, int horizontal, int vertical,
                                   float scale, unsigned int layer, unsigned int flags);
    using init_text_t = bm2dx::text_props_t* (*)(bm2dx::text_props_t* properties);
    using text_render_t = void (*)(int font, int horizontal, int vertical, int layer,
                                  bm2dx::text_props_t* properties, const char* text);

    judge_apply_t original_judge_apply_fn = nullptr;
    judge_display_t original_judge_display_fn = nullptr;
    draw_indicator_t original_judge_draw_fs_keys_fn = nullptr;
    draw_indicator_t original_judge_draw_fs_sc_fn = nullptr;
    judge_display_init_t original_judge_display_init_fn = nullptr;
    sprite_draw_t original_sprite_draw_fn = nullptr;

    // Shared timing and enable state are accessed from both game hooks and the menu.
    auto state_mutex = std::mutex {};
    auto cache = fast_slow_display::display_cache_t {};
    auto display_mode = fast_slow_display::mode_t::off;
    auto installed = false;

    auto get_mode() -> fast_slow_display::mode_t
    {
        const auto lock = std::lock_guard { state_mutex };
        return display_mode;
    }

    auto is_enabled() -> bool
    {
        return get_mode() != fast_slow_display::mode_t::off;
    }

    // Stores the pending judgment for next draw call.
    struct pending_judgment_t
    {
        int player = -1;
        bool scratch = false;
        timing_t timing;
    };
    thread_local auto pending = pending_judgment_t {};

    // Temporary state for the next draw function.
    struct render_context_t
    {
        timing_t timing;
        std::array<char, 7> text {};
        bool word_label = false;
    };
    thread_local auto rendering = render_context_t {};

    // Helper routine for reading bytes from base address + offset.
    template<typename Value>
    auto read(const void* object, const std::size_t offset = 0) -> Value
    {
        auto value = Value {};
        std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
        return value;
    }

    // Determines if a user has separated fast/slow display option enabled.
    auto separate_scratch(const int player) -> bool
    {
        constexpr auto profiles_offset = 0x08;
        constexpr auto profile_stride = 0xac;
        constexpr auto separate_scratch_offset = 0x64;

        // Profiles 0/1 are P1/P2 single play.
        // Profile 2 is double play.
        const auto profile = bm2dx::state->play_style != 0 ? 2 : player;

        // A value of 1 selects independent key/scratch timing.
        const auto game_options = reinterpret_cast<void* (*)()>(bm2dx::addr->GET_PLAY_OPTIONS_FN)();

        return read<int>(game_options, profiles_offset + profile_stride * profile + separate_scratch_offset) == 1;
    }

    // Called to reset judge display for a player.
    auto judge_display_init_hook_fn(void* display, const int player, const int mode) -> std::intptr_t
    {
        {
            const auto lock = std::lock_guard { state_mutex };
            cache.reset(player, reinterpret_cast<std::uintptr_t>(display));
        }
        return original_judge_display_init_fn(display, player, mode);
    }

    // Timing value must be captured here, before the original call updates the display.
    auto judge_apply_hook_fn(void* context, const int player, const int grade,
                             const int lane, const int score_index) -> int
    {
#ifdef _MSC_VER
        const auto caller = static_cast<std::uint8_t*>(_ReturnAddress());
#else
        const auto caller = static_cast<std::uint8_t*>(__builtin_return_address(0));
#endif

        // Load the saved pending value
        const auto saved_pending = pending;
        pending = {};

        try
        {
            const auto& addresses = *bm2dx::addr;

            const bool capture_enabled = is_enabled();
            const bool valid_note = player >= 0 && player < 2 && lane >= 0 && lane < 8;
            const bool is_press = caller == addresses.JUDGE_PRESS_RETURN;
            const bool is_release = caller == addresses.JUDGE_RELEASE_RETURN;

            if (capture_enabled && valid_note && (is_press || is_release))
            {
                // read the candidate judgment context for this lane and player
                const auto candidate = read<judge_apply_hook_context_t>(
                    context, sizeof(judge_apply_hook_context_t) * (lane + 8 * player));
                if (candidate.note)
                {
                    const bool is_scratch = lane == 7;

                    // note+24 is a flag that tells us if the note has not been judged
                    // if it was already judged (0) then this is an excessive poor
                    // the game engine would normally display SLOW for this and generate +250ms
                    // as a placeholder but showing that ms value would be misleading; therefore
                    // we do a special case for this (show "poor")
                    const bool excessive_poor = is_press &&
                        grade == bm2dx::judge_grade::late_poor &&
                        read<std::uint8_t>(candidate.note, 24) == 0;

                    pending =
                    {
                        player,
                        is_scratch,
                        { candidate.milliseconds, excessive_poor }
                    };
                }
            }

            // call into the original judgment apply function
            const auto result = original_judge_apply_fn(context, player, grade, lane, score_index);
            pending = saved_pending;
            return result;
        }
        catch (...)
        {
            pending = saved_pending;
            throw;
        }
    }

    // Hook for judge display.
    // This doesn't draw anything, but it is hooked store the timing value for later use by draw function.
    auto judge_display_hook_fn(void* display, const int code, const int combo,
                               const bool scratch) -> std::intptr_t
    {
        const auto result = original_judge_display_fn(display, code, combo, scratch);
        const auto player = read<judge_display_hook_context_t>(display).player;

        const auto lock = std::lock_guard { state_mutex };
        auto timing = timing_t {};
        if (display_mode != fast_slow_display::mode_t::off &&
            pending.player == player && pending.scratch == scratch)
            timing = pending.timing;

        cache.update(player, reinterpret_cast<std::uintptr_t>(display), code, scratch, timing);
        return result;
    }

    // The game still decides whether and where to draw an indicator. We prepare the text here,
    // then let its draw function reach sprite_draw_hook_fn with the native position and layer.
    auto draw_indicator(void* display, const bool scratch, const draw_indicator_t original) -> void
    {
        const auto saved_rendering = rendering;
        rendering = {};

        try
        {
            // if the feature is disabled, call the original
            const auto mode = get_mode();
            if (mode == fast_slow_display::mode_t::off)
            {
                original(display);
                rendering = saved_rendering;
                return;
            }

            auto local_display = read<judge_display_hook_context_t>(display);
            auto pgreat = false;
            if (local_display.player >= 0 && local_display.player < 2)
            {
                const auto separate = separate_scratch(local_display.player);
                auto* displayed_code = &local_display.combined_code;
                if (scratch)
                    displayed_code = &local_display.scratch_code;
                else if (separate)
                    displayed_code = &local_display.key_code;

                const auto code = *displayed_code;
                if (mode == fast_slow_display::mode_t::great_and_below &&
                    code == bm2dx::judge_display_code::pgreat)
                {
                    rendering = saved_rendering;
                    return;
                }

                {
                    const auto lock = std::lock_guard { state_mutex };
                    rendering.timing = cache.get(local_display.player, reinterpret_cast<std::uintptr_t>(display),
                                                 separate, scratch);
                }

                if (code == bm2dx::judge_display_code::miss && rendering.timing.excessive_poor)
                {
                    rendering.word_label = true;
                    rendering.text = { 'p', 'o', 'o', 'r', '\0' };
                }
                else if (code == bm2dx::judge_display_code::miss &&
                    rendering.timing.get_polarity() == polarity::zero)
                {
                    // For misses without measured timing, display 'miss'.
                    rendering.word_label = true;
                    rendering.timing = {};
                    rendering.text = { 'm', 'i', 's', 's', '\0' };
                }
                else
                {
                    // Otherwise, format the text for millisecond timing.
                    rendering.text = fast_slow_display::format_fastslow_ms(rendering.timing.milliseconds);
                }

                if (code == bm2dx::judge_display_code::pgreat && rendering.text[0] != '\0')
                {
                    // Code 4 (PGREAT) normally emits no FAST/SLOW sprite for us to replace.
                    // Change only our copy to code 3 (FAST) or 5 (SLOW), making the native draw
                    // function emit that sprite while leaving the game's real PGREAT untouched.
                    *displayed_code = rendering.timing.get_polarity() == polarity::slow ?
                        bm2dx::judge_display_code::late_great : bm2dx::judge_display_code::early_great;
                    pgreat = true;
                }
            }

            if (rendering.text[0] == '\0')
            {
                rendering = saved_rendering;
                return;
            }

            if (pgreat)
            {
                // Call the original but convince it to still display f/s indicator even for pgreat
                original(&local_display);
            }
            else
            {
                // Call the original.
                original(display);
            }

            // Restore the original rendering state.
            rendering = saved_rendering;
        }
        catch (...)
        {
            rendering = saved_rendering;
            throw;
        }
    }

    auto judge_draw_fs_keys_hook_fn(void* display) -> void
    {
        draw_indicator(display, false, original_judge_draw_fs_keys_fn);
    }

    auto judge_draw_fs_sc_hook_fn(void* display) -> void
    {
        draw_indicator(display, true, original_judge_draw_fs_sc_fn);
    }

    auto get_text_color(const bool fast, const bool scratch) -> const char*
    {
        // slightly brighter versions for scratch
        if (scratch) {
            return fast ? "99e6ffff" : "ffb3b3ff";
        }

        // same color as fast/slow sprites
        return fast ? "3399ffff" : "ff3333ff";
    }

    auto draw_text(int horizontal, int vertical, const unsigned int layer, const char* text,
                   const int width, const int height, const bool fast, const bool scratch,
                   const bool word_label) -> void
    {
        // Font 3 is DFG Heisei Gothic W7 at 16 pixels, also used by bottom text like FREE PLAY.
        constexpr auto font = 3;

        // Five characters plus boldness and border occupy 63 pixels
        // compare to 66-pixel width of FAST / SLOW sprites.
        constexpr auto cell_width = 12;

        // This ends up being 24px tall, which is identical to FAST / SLOW sprites.
        constexpr auto cell_height = 22;

        constexpr auto border_radius = 1;
        constexpr auto bold_offset = 1;

        auto properties = bm2dx::text_props_t {};

        // ask game to init text
        static auto init_text = reinterpret_cast<init_text_t>(bm2dx::addr->TEXT_INIT_FN);
        init_text(&properties);

        auto content = std::string {};
        if (word_label)
        {
            // Center the whole word in the sprite area, with all letters on a shared baseline.
            properties.h_align = 1;
            properties.v_align = 1;
            horizontal += (width - bold_offset) / 2;
            vertical += height / 2;
            content = text;
        }
        else
        {
            const auto text_width = static_cast<int>(std::strlen(text)) * cell_width;
            const auto total_width = text_width + bold_offset + 2 * border_radius;
            const auto total_height = cell_height + 2 * border_radius;

            // Right-align fixed-width cells so the decimal point stays put as integer digits change.
            horizontal += width - total_width + border_radius;
            vertical += (height - total_height) / 2 + border_radius;
            content = fmt::format("<tt {} {}>{}</tt>", cell_width, cell_height, text);
        }

        // Format string.
        const auto label = fmt::format(
            "<color {}><scale 1.0 1.0>{}</scale></color>",
            get_text_color(fast, scratch),
            content);
        const auto border = fmt::format(
            "<color 000000ff><scale 1.0 1.0>{}</scale></color>", content);

        static auto text_render = reinterpret_cast<text_render_t>(bm2dx::addr->TEXT_RENDER_FN);
        for (int offset_y = -border_radius; offset_y <= border_radius; ++offset_y)
        {
            for (int offset_x = -border_radius; offset_x <= bold_offset + border_radius; ++offset_x)
            {
                if (offset_y == 0 && offset_x >= 0 && offset_x <= bold_offset)
                    continue;

                text_render(font, horizontal + offset_x, vertical + offset_y,
                            static_cast<int>(layer), &properties, border.c_str());
            }
        }
        text_render(font, horizontal, vertical, static_cast<int>(layer), &properties, label.c_str());
        text_render(font, horizontal + bold_offset, vertical, static_cast<int>(layer), &properties, label.c_str());
    }

    // Replace only FAST/SLOW sprites emitted within the current indicator draw call.
    auto sprite_draw_hook_fn(void* manager, const char* name, const int horizontal, const int vertical,
                             const float scale, const unsigned int layer, const unsigned int flags) -> void*
    {
        const auto sprite = original_sprite_draw_fn(manager, name, horizontal, vertical, scale, layer, flags);
        if (!sprite || !name || rendering.text[0] == '\0')
            return sprite;

        const auto asset = std::string_view { name };
        if (asset != "fast" && asset != "slow" && asset != "s_fast" && asset != "s_slow")
            return sprite;

        const auto vtable = read<std::uintptr_t*>(sprite);
        const auto set_visible = reinterpret_cast<void (*)(void*, bool)>(vtable[5]);
        const auto get_dimensions = reinterpret_cast<std::intptr_t (*)(void*, int*, int*)>(vtable[39]);

        // Hide the original before drawing the replacement. Even if text drawing
        // fails, enabled mode must not briefly show the native FAST/SLOW label instead.
        set_visible(sprite, false);

        auto width = 0;
        auto height = 0;
        get_dimensions(sprite, &width, &height);
        draw_text(horizontal, vertical, layer, rendering.text.data(), width, height,
                  rendering.timing.get_polarity() == polarity::fast, asset.starts_with("s_"), rendering.word_label);

        return sprite;
    }

    // Menu-facing controls. Toggling starts with an empty timing cache.
    auto available() -> bool
    {
        const auto lock = std::lock_guard { state_mutex };
        return installed;
    }

    auto set_mode(const fast_slow_display::mode_t value) -> void
    {
        const auto lock = std::lock_guard { state_mutex };
        display_mode = installed ? value : fast_slow_display::mode_t::off;
        cache = {};
    }

    auto install_hook() -> void
    {
        // Create the complete hook chain here; common startup enables it with the other hooks.
        const auto& addresses = *bm2dx::addr;
        const auto targets = std::array {
            addresses.JUDGE_APPLY_FN,
            addresses.JUDGE_DISPLAY_FN,
            addresses.JUDGE_DRAW_FS_KEYS_FN,
            addresses.JUDGE_DRAW_FS_SC_FN,
            addresses.JUDGE_DISPLAY_INIT_FN,
            addresses.SPRITE_DRAW_FN
        };

        const auto results = std::array {
            MH_CreateHook(
                addresses.JUDGE_APPLY_FN,
                reinterpret_cast<LPVOID>(judge_apply_hook_fn),
                reinterpret_cast<LPVOID*>(&original_judge_apply_fn)),
            MH_CreateHook(
                addresses.JUDGE_DISPLAY_FN,
                reinterpret_cast<LPVOID>(judge_display_hook_fn),
                reinterpret_cast<LPVOID*>(&original_judge_display_fn)),
            MH_CreateHook(
                addresses.JUDGE_DRAW_FS_KEYS_FN,
                reinterpret_cast<LPVOID>(judge_draw_fs_keys_hook_fn),
                reinterpret_cast<LPVOID*>(&original_judge_draw_fs_keys_fn)),
            MH_CreateHook(
                addresses.JUDGE_DRAW_FS_SC_FN,
                reinterpret_cast<LPVOID>(judge_draw_fs_sc_hook_fn),
                reinterpret_cast<LPVOID*>(&original_judge_draw_fs_sc_fn)),
            MH_CreateHook(
                addresses.JUDGE_DISPLAY_INIT_FN,
                reinterpret_cast<LPVOID>(judge_display_init_hook_fn),
                reinterpret_cast<LPVOID*>(&original_judge_display_init_fn)),
            MH_CreateHook(
                addresses.SPRITE_DRAW_FN,
                reinterpret_cast<LPVOID>(sprite_draw_hook_fn),
                reinterpret_cast<LPVOID*>(&original_sprite_draw_fn))
        };

        // Capture, cache updates, and sprite replacement must be installed together. If any
        // creation failed, remove only hooks created by this attempt and keep the option disabled.
        for (const auto status : results)
        {
            if (status != MH_OK)
            {
                for (std::size_t index = 0; index < targets.size(); ++index)
                    if (results[index] == MH_OK)
                        MH_RemoveHook(targets[index]);
                return;
            }
        }

        const auto lock = std::lock_guard { state_mutex };
        installed = true;
    }
}