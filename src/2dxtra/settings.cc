#include <meta.h>
#include <cmath>
#include <limits>
#include <type_traits>
#include "settings.h"
#include "gui/gui.h"
#include "hooks/reset_state_hook.h"
#include "features/autoplay.h"
#include "features/autoretry.h"
#include "features/chart_speed.h"
#include "features/cn_override.h"
#include "features/cn_transformer.h"
#include "features/fast_slow_display.h"
#include "features/hi_speed_reset.h"
#include "features/keysound_switch.h"
#include "features/play_visuals.h"
#include "features/regular_speed.h"
#include "features/scratch_swap.h"
#include "features/timing_histogram.h"
#include "features/timing_modifier.h"
#include "features/unrandomizer.h"

namespace iidxtra::settings
{
    static database::db* storage = nullptr;

    template <typename Action>
    static auto for_each_timing_setting(Action& apply, const std::string& prefix,
                                       const std::string& suffix, bm2dx::timing_t& timing) -> void
    {
        constexpr auto limit = std::numeric_limits<float>::max();
        apply(prefix + ".early_poor" + suffix, timing.early_poor, -limit, limit);
        apply(prefix + ".early_bad" + suffix, timing.early_bad, -limit, limit);
        apply(prefix + ".early_good" + suffix, timing.early_good, -limit, limit);
        apply(prefix + ".early_great" + suffix, timing.early_great, -limit, limit);
        apply(prefix + ".early_pgreat" + suffix, timing.early_pgreat, -limit, limit);
        apply(prefix + ".late_pgreat" + suffix, timing.late_pgreat, -limit, limit);
        apply(prefix + ".late_great" + suffix, timing.late_great, -limit, limit);
        apply(prefix + ".late_good" + suffix, timing.late_good, -limit, limit);
        apply(prefix + ".late_bad" + suffix, timing.late_bad, -limit, limit);
    }

    template <typename Action>
    static auto for_each_setting(Action apply) -> void
    {
        apply("interface.menu_lock", gui::play_lock_state);
        apply("hi_speed_reset.enabled", hi_speed_reset::enabled);

        #if FORCE_EVENT_MODE_ENABLED == 0
        apply("settings.event_mode", gui::config_event_mode);
        #endif

        apply("regular_speed.enabled", regular_speed::enabled);
        apply("chart_speed.rate", chart_speed::rate, chart_speed::rate_min, chart_speed::rate_max);
        apply("chart_speed.previous", chart_speed::rate_previous, chart_speed::rate_min, chart_speed::rate_max);
        apply("chart_speed.pitch", chart_speed::pitch_follows_rate);

        apply("keysound.mode", keysound_switch::override_type, 0, 3);
        apply("keysound.mute_bgm", keysound_switch::mute_bgm);

        apply("visuals.dark_mode", play_visuals::dark_mode);
        apply("visuals.no_measure_lines", play_visuals::no_measure_lines);
        apply("visuals.no_bpm_gradient", play_visuals::no_bpm_gradient);
        apply("fast_slow.milliseconds", fast_slow_display::options.show_milliseconds);
        apply("fast_slow.pgreat", fast_slow_display::options.show_pgreat);
        apply("timing_histogram.enabled", timing_histogram::enabled);

        apply("autoretry.enabled", autoretry::enabled);
        apply("autoretry.target", autoretry::target, 0, 2);
        apply("autoretry.destination", autoretry::destination, 0, 1);

        apply("autoplay.p1", autoplay::enabled_p1);
        apply("autoplay.p2", autoplay::enabled_p2);

        apply("cn_override.p1", cn_override::p1_override_type, 0, 2);
        apply("cn_override.p2", cn_override::p2_override_type, 0, 2);
        apply("cn.enabled.p1", cn_transformer::enabled_p1);
        apply("cn.enabled.p2", cn_transformer::enabled_p2);
        apply("cn.percentage.p1", cn_transformer::long_note_percentage_p1, 5, 100);
        apply("cn.percentage.p2", cn_transformer::long_note_percentage_p2, 5, 100);
        apply("cn.duration.p1", cn_transformer::max_cn_duration_p1, -1, 10000);
        apply("cn.duration.p2", cn_transformer::max_cn_duration_p2, -1, 10000);
        apply("cn.scratch.p1", cn_transformer::allow_backspin_scratch_p1);
        apply("cn.scratch.p2", cn_transformer::allow_backspin_scratch_p2);

        apply("random.enabled.p1", unrandomizer::enabled_p1);
        apply("random.enabled.p2", unrandomizer::enabled_p2);

        #if BLOCK_FORCE_RANDOM_SAVE == 0
        apply("random.save_scores.p1", unrandomizer::allow_score_save_p1);
        apply("random.save_scores.p2", unrandomizer::allow_score_save_p2);
        #endif

        apply("random.show_info.p1", unrandomizer::show_random_info_p1);
        apply("random.show_info.p2", unrandomizer::show_random_info_p2);
        apply("random.hran.p1", unrandomizer::hran_enabled_p1);
        apply("random.hran.p2", unrandomizer::hran_enabled_p2);
        apply("scratch_swap.enabled.p1", scratch_swap::enabled_p1);
        apply("scratch_swap.enabled.p2", scratch_swap::enabled_p2);
        apply("scratch_swap.lane.p1", scratch_swap::swap_lane_p1, -1, 6);
        apply("scratch_swap.lane.p2", scratch_swap::swap_lane_p2, -1, 6);

        for (std::size_t lane = 0; lane < unrandomizer::column_lut_p1.size(); ++lane)
        {
            apply("random.lane" + std::to_string(lane) + ".p1", unrandomizer::column_lut_p1[lane], 0, 6);
            apply("random.lane" + std::to_string(lane) + ".p2", unrandomizer::column_lut_p2[lane], 0, 6);
        }

        apply("timing.keys.p1", timing_modifier::enabled_keys[0]);
        apply("timing.keys.p2", timing_modifier::enabled_keys[1]);
        apply("timing.scratch.p1", timing_modifier::enabled_scratch[0]);
        apply("timing.scratch.p2", timing_modifier::enabled_scratch[1]);
        for_each_timing_setting(apply, "timing.keys", ".p1", timing_modifier::override[0].keys);
        for_each_timing_setting(apply, "timing.keys", ".p2", timing_modifier::override[1].keys);
        for_each_timing_setting(apply, "timing.scratch", ".p1", timing_modifier::override[0].scratch);
        for_each_timing_setting(apply, "timing.scratch", ".p2", timing_modifier::override[1].scratch);
    }

    static auto is_valid_setting_value(double value, double minimum, double maximum) -> bool
    {
        if (!std::isfinite(value))
            return false;

        if (value < minimum || value > maximum)
            return false;

        return true;
    }

    template <typename Setting>
    static auto load_setting_value(Setting& setting, const database::setting_value& saved,
                                   double minimum, double maximum) -> void
    {
        if constexpr (std::is_floating_point_v<Setting>)
        {
            auto const* value = std::get_if<double>(&saved);
            if (value != nullptr && is_valid_setting_value(*value, minimum, maximum))
                setting = static_cast<Setting>(*value);
        }
        else
        {
            auto const* value = std::get_if<std::int64_t>(&saved);
            if (value != nullptr && is_valid_setting_value(static_cast<double>(*value), minimum, maximum))
                setting = static_cast<Setting>(*value);
        }
    }

    template <typename Setting>
    static auto save_setting_value(Setting setting) -> database::setting_value
    {
        if constexpr (std::is_floating_point_v<Setting>)
            return static_cast<double>(setting);
        else
            return static_cast<std::int64_t>(setting);
    }

    auto init(database::db* db) -> void
    {
        storage = db;

        // Load settings from the database
        auto const values = database::load_settings(storage);
        if (values.empty())
            return;

        // Initialize settings from values loaded from the database
        for_each_setting([&](const std::string& key, auto& setting, double min = 0, double max = 1)
        {
            for (auto const& [saved_key, saved_value] : values)
            {
                if (saved_key != key)
                    continue;

                // Depending on the type of the setting, assign the value if it is valid
                load_setting_value(setting, saved_value, min, max);
                break;
            }
        });

        // Reset unrandomizer if the loaded state is invalid
        if (!unrandomizer::is_valid(0) || !unrandomizer::is_valid(1))
            unrandomizer::reset();

        // Update runtime settings
        play_visuals::update_dark_mode();
        play_visuals::update_no_measure_lines();
        play_visuals::update_no_bpm_gradient();
        fast_slow_display::update();
        timing_histogram::update();
    }

    auto save() -> bool
    {
        database::settings_t values;
        for_each_setting([&](const std::string& key, const auto& setting, double = 0, double = 1)
        {
            values.emplace_back(key, save_setting_value(setting));
        });
        return database::save_settings(storage, values);
    }

    auto reset() -> void
    {

        #if FORCE_EVENT_MODE_ENABLED
        gui::config_event_mode = true;
        #else 
        gui::config_event_mode = false;
        #endif

        reset_state_hook::reset_options();
    }
}