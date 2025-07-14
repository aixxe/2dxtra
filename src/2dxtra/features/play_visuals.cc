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
        auto static dark_mode_patch = util::code_patch
            { bm2dx::addr->DARK_MODE_PATCH, { 0x90, 0x90 } };

        dark_mode ?
            dark_mode_patch.enable():
            dark_mode_patch.disable();
    }

    auto update_no_measure_lines() -> void
    {
        auto static no_measure_lines_patch = util::code_patch
            { bm2dx::addr->MEASURE_PATCH, { 0xEB } };

        no_measure_lines ?
            no_measure_lines_patch.enable():
            no_measure_lines_patch.disable();
    }

    auto update_no_bpm_gradient() -> void
    {
        // todo: anything but this
        auto static no_bpm_gradient_patch = util::code_patch { bm2dx::addr->BPM_BAR_PATCH, {
            0xE9,
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >>  0 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >>  8 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >> 16 & 0xFF),
            static_cast<std::uint8_t>(bm2dx::addr->BPM_BAR_PATCH_JMP >> 24 & 0xFF),
        } };

        no_bpm_gradient ?
            no_bpm_gradient_patch.enable():
            no_bpm_gradient_patch.disable();
    }
}
