#pragma once

#include "database.h"

namespace iidxtra::settings
{
    auto init(database::db*) -> void;
    auto save() -> bool;
    auto reset() -> void;
}