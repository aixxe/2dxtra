#pragma once

namespace iidxtra::play_visuals
{
    extern bool dark_mode;
    extern bool no_measure_lines;
    extern bool no_bpm_gradient;

    auto reset() -> void;
    auto update_dark_mode() -> void;
    auto update_no_measure_lines() -> void;
    auto update_no_bpm_gradient() -> void;
}