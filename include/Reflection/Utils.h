#pragma once
#include <cmath>
#include <functional>

namespace Gleam::Reflection::Utils {

template<typename T = void>
static constexpr T* OffsetPointer(void* ptr, size_t offset)
{
    ptr = static_cast<char*>(ptr) + offset;
    return static_cast<T*>(ptr);
}

template<typename T = void>
static constexpr const T* OffsetPointer(const void* ptr, size_t offset)
{
    ptr = static_cast<const char*>(ptr) + offset;
    return static_cast<const T*>(ptr);
}

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

template <typename T>
constexpr void HashCombine(size_t& seed, const T& value)
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
