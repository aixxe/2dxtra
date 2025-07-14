#pragma once

namespace iidxtra::autoretry
{
    enum class target_mode { Off, Delta, Maximum };
    enum class target_destination { Ghost, Graph };

    extern bool enabled;

    extern target_mode target;
    extern target_destination destination;

    auto reset() -> void;

    auto install_hook() -> void;
}