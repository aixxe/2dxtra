#pragma once

#include <cstdint>

#include "../database.h"

namespace iidxtra::chart_analyze_hook
{
    auto install(database::db* cache) -> void;
}
