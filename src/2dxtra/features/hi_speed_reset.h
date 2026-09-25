#pragma once

namespace iidxtra::hi_speed_reset
{
    extern bool enabled;

    auto reset() -> void;
    auto install_hook() -> void;
}
