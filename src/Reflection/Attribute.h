#pragma once
#include "Utils.h"
#include "Macro.h"

namespace Gleam::Reflection {

struct AttributeDescription
{
    const char* tag;
    uint32_t hash;
    
    template<size_t N>
    explicit constexpr AttributeDescription(const char (&str)[N])
        : tag(str), hash(Utils::HashString(str))
    {
    }
    
    explicit constexpr AttributeDescription(const char* str)
        : tag(str), hash(Utils::HashString(str))
    {
    }
};

namespace Attribute {

GLEAM_ATTRIBUTE(Guid)
{
    uint8_t bytes[16];
    
    explicit constexpr Guid()
        : bytes{0}
    {
        
    }
    
    template<size_t N>
    explicit constexpr Guid(const char (&str)[N])
        : bytes{
        static_cast<uint8_t>((Utils::HexDigitToByte(str[6]) << 4) | (Utils::HexDigitToByte(str[7]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[4]) << 4) | (Utils::HexDigitToByte(str[5]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[2]) << 4) | (Utils::HexDigitToByte(str[3]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[0]) << 4) | (Utils::HexDigitToByte(str[1]) << 0)),
        
        // str[8] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[11]) << 4) | (Utils::HexDigitToByte(str[12]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[9]) << 4) | (Utils::HexDigitToByte(str[10]) << 0)),
        
        // str[13] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[16]) << 4) | (Utils::HexDigitToByte(str[17]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[14]) << 4) | (Utils::HexDigitToByte(str[15]) << 0)),
        
        // str[18] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[19]) << 4) | (Utils::HexDigitToByte(str[20]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[21]) << 4) | (Utils::HexDigitToByte(str[22]) << 0)),
        
        // str[23] = separator
        static_cast<uint8_t>((Utils::HexDigitToByte(str[24]) << 4) | (Utils::HexDigitToByte(str[25]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[26]) << 4) | (Utils::HexDigitToByte(str[27]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[28]) << 4) | (Utils::HexDigitToByte(str[29]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[30]) << 4) | (Utils::HexDigitToByte(str[31]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[32]) << 4) | (Utils::HexDigitToByte(str[33]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[34]) << 4) | (Utils::HexDigitToByte(str[35]) << 0))}
    {
        static_assert(N == 37); // 32 bytes + 4 separators + null terminator
    }
    
    explicit constexpr Guid(const char* str)
        : bytes{
        static_cast<uint8_t>((Utils::HexDigitToByte(str[6]) << 4) | (Utils::HexDigitToByte(str[7]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[4]) << 4) | (Utils::HexDigitToByte(str[5]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[2]) << 4) | (Utils::HexDigitToByte(str[3]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[0]) << 4) | (Utils::HexDigitToByte(str[1]) << 0)),
        
        // str[8] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[11]) << 4) | (Utils::HexDigitToByte(str[12]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[9]) << 4) | (Utils::HexDigitToByte(str[10]) << 0)),
        
        // str[13] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[16]) << 4) | (Utils::HexDigitToByte(str[17]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[14]) << 4) | (Utils::HexDigitToByte(str[15]) << 0)),
        
        // str[18] = separator
        
        static_cast<uint8_t>((Utils::HexDigitToByte(str[19]) << 4) | (Utils::HexDigitToByte(str[20]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[21]) << 4) | (Utils::HexDigitToByte(str[22]) << 0)),
        
        // str[23] = separator
        static_cast<uint8_t>((Utils::HexDigitToByte(str[24]) << 4) | (Utils::HexDigitToByte(str[25]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[26]) << 4) | (Utils::HexDigitToByte(str[27]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[28]) << 4) | (Utils::HexDigitToByte(str[29]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[30]) << 4) | (Utils::HexDigitToByte(str[31]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[32]) << 4) | (Utils::HexDigitToByte(str[33]) << 0)),
        static_cast<uint8_t>((Utils::HexDigitToByte(str[34]) << 4) | (Utils::HexDigitToByte(str[35]) << 0))}
    {
        
    }
    
    static constexpr Guid InvalidGuid()
    {
        return Guid();
    }
    
    bool operator==(const Guid& other) const
    {
        return memcmp(bytes, other.bytes, sizeof(bytes));
    }
    
    bool operator!=(const Guid& other) const
    {
        return !((*this) == other);
    }
};

GLEAM_ATTRIBUTE(Version)
{
    uint32_t version;
    
    explicit constexpr Version(uint32_t version)
    : version(version)
    {
        
    }
};

GLEAM_ATTRIBUTE(EntityComponent)
{
};

GLEAM_ATTRIBUTE(Serializable)
{
};

GLEAM_ATTRIBUTE(PrettyName)
{
    const char* name;
    
    explicit constexpr PrettyName(const char* name)
    : name(name)
    {
        
    }
};

} // namespace Attribute

} // namespace Gleam::Reflection
