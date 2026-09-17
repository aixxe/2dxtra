#pragma once

namespace bm2dx::judge_grade
{
    constexpr int early_poor = 0;
    constexpr int early_bad = 1;
    constexpr int early_good = 2;
    constexpr int early_great = 3;
    constexpr int early_pgreat = 4;
    constexpr int late_pgreat = 5;
    constexpr int late_great = 6;
    constexpr int late_good = 7;
    constexpr int late_bad = 8;
    constexpr int late_poor = 9;
}

namespace bm2dx::judge_display_code
{
    // Display codes merge the two PGREAT grades and several POOR variants.
    constexpr int early_poor = 0;
    constexpr int early_bad = 1;
    constexpr int early_good = 2;
    constexpr int early_great = 3;
    constexpr int pgreat = 4;
    constexpr int late_great = 5;
    constexpr int late_good = 6;
    constexpr int late_bad = 7;
    constexpr int miss = 8;
    constexpr int charge_hold = 12;
}