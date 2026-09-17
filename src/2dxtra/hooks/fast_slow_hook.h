#pragma once

#include "../features/fast_slow_display.h"

namespace iidxtra::fast_slow_hook
{
    auto install_hook() -> void;
    auto available() -> bool;
    auto set_options(fast_slow_display::options_t value) -> void;
}