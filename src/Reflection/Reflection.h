#pragma once
#include "Database.h"

namespace Gleam::Reflection {

static constexpr PrimitiveType GetPrimitiveType(uint32_t hash)
{
    return static_cast<PrimitiveType>(hash);
}

} // namespace Gleam::Reflection
