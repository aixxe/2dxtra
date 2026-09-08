#pragma once

namespace iidxtra::fast_slow_hook
{
    auto install_hook() -> void;
    auto available() -> bool;
    auto set_enabled(bool value) -> void;
}