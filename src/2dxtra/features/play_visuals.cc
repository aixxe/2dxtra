#include "../game.h"
#include "../util/code_patch.h"
#include "play_visuals.h"

namespace iidxtra::play_visuals
{
    auto dark_mode = false;
    auto no_measure_lines = false;
    auto no_bpm_gradient = false;

    auto reset() -> void
    {
        dark_mode = false;
        no_measure_lines = false;
        no_bpm_gradient = false;

        update_dark_mode();
        update_no_measure_lines();
        update_no_bpm_gradient();
    }

    auto update_dark_mode() -> void
    {
        auto static patch = util::branch_patch { bm2dx::addr->DARK_MODE_PATCH };
        dark_mode ? patch.enable(): patch.disable();
    }

    auto update_no_measure_lines() -> void
    {
        auto static patch = util::branch_patch { bm2dx::addr->MEASURE_PATCH };

        no_measure_lines ? patch.enable(): patch.disable();
    }

    auto update_no_bpm_gradient() -> void
    {
        auto static patch = util::code_patch { bm2dx::addr->BPM_BAR_PATCH, {
            0xE9,
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >>  0 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >>  8 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >> 16 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >> 24 & 0xFF),
        } };

        no_bpm_gradient ? patch.enable(): patch.disable();
    }
}
