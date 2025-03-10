#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace Gleam::Reflection::Utils {

static constexpr uint8_t HexDigitToByte(const char ch)
{
    // 0-9
    if (ch >= '0' && ch <= '9')
        return uint8_t(ch - '0');

    // a-f
    if (ch >= 'a' && ch <= 'f')
        return uint8_t(10 + ch - 'a');

    // A-F
    if (ch >= 'A' && ch <= 'F')
        return uint8_t(10 + ch - 'A');

    return uint8_t(0);
}

// Fowler–Noll–Vo hash
template<size_t N>
static constexpr uint32_t HashString(const char (&str)[N])
{
    constexpr uint32_t FNV_PRIME = 16777619u;
    constexpr uint32_t OFFSET_BASIS = 2166136261u;
    
    uint32_t hash = OFFSET_BASIS;
    for (uint32_t i = 0; i < N; ++i)
    {
        hash ^= static_cast<uint32_t>(str[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

static constexpr uint32_t HashString(const char* str)
{
    constexpr uint32_t FNV_PRIME = 16777619u;
    constexpr uint32_t OFFSET_BASIS = 2166136261u;
    
    uint32_t hash = OFFSET_BASIS;
    while (*str)
    {
        hash ^= static_cast<uint32_t>(*str);
        hash *= FNV_PRIME;
        ++str;
    }
    return hash;
}
    
} // namespace Gleam
