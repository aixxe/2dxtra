#pragma once

namespace iidxtra::autoplay
{
    extern bool enabled_p1;
    extern bool enabled_p2;

    auto reset() -> void;

    auto install_hook() -> void;
}