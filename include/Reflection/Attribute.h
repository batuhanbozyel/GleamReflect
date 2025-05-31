#pragma once
#include "Utils.h"
#include "Macro.h"

#include <iomanip>
#include <string>
#include <regex>
#include <ios>

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

struct IAttribute {};

namespace Attribute {

GLEAM_ATTRIBUTE(Guid)
{
    union
    {
        struct
        {
            uint32_t mData1;
            uint16_t mData2;
            uint16_t mData3;
            uint8_t mData4[8];
        };
        uint8_t mBytes[16];
    };
    
    explicit constexpr Guid()
        : mBytes{0}
    {
        
    }
    
    template<size_t N>
    explicit constexpr Guid(const char (&str)[N])
        : mBytes{
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
        : mBytes{
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

    explicit Guid(const std::string& args)
        : Guid()
    {
        std::regex guidRegex("\\{?([0-9a-fA-F]{8})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{12})\\}?");
        std::smatch matches;
        if (std::regex_search(args, matches, guidRegex) && matches.size() == 6)
        {
            // Format the GUID in the standard format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
            std::string formattedGuid =
                matches[1].str() + "-" +
                matches[2].str() + "-" +
                matches[3].str() + "-" +
                matches[4].str() + "-" +
                matches[5].str();
            
            *this = Guid(formattedGuid.c_str());
        }
    }

    constexpr Guid(const Guid&) = default;
    constexpr Guid& operator=(const Guid&) noexcept = default;

    constexpr Guid(Guid&&) = default;
    constexpr Guid& operator=(Guid&&) noexcept = default;
    
    static Guid InvalidGuid()
    {
        return Guid();
    }
    
    bool operator==(const Guid& other) const
    {
        return memcmp(mBytes, other.mBytes, sizeof(mBytes)) == 0;
    }
    
    bool operator!=(const Guid& other) const
    {
        return !((*this) == other);
    }

	std::string ToString() const
	{
		std::string str;
		str.resize(36);
		snprintf(str.data(),
				str.length() + 1,
				"%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
				mData1,
				mData2,
				mData3,
				mData4[0],
				mData4[1],
				mData4[2],
				mData4[3],
				mData4[4],
				mData4[5],
				mData4[6],
				mData4[7]);
		return str;
	}

	operator std::string() const
    {
        return ToString();
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
    std::string_view name;
    
    explicit constexpr PrettyName(const std::string_view name)
        : name(name)
    {
        
    }
};

} // namespace Attribute

template<typename T>
concept AttributeType = std::is_base_of_v<IAttribute, T>;

} // namespace Gleam::Reflection

namespace std {

template <>
struct hash<Gleam::Reflection::Attribute::Guid>
{
	size_t operator()(const Gleam::Reflection::Attribute::Guid& guid) const
	{
		size_t hash = 0;
		Gleam::Reflection::Utils::HashCombine(hash, guid.mData1);
		Gleam::Reflection::Utils::HashCombine(hash, guid.mData2);
		Gleam::Reflection::Utils::HashCombine(hash, guid.mData3);

		uint64_t data4;
		memcpy(&data4, guid.mData4, sizeof(uint64_t));
		Gleam::Reflection::Utils::HashCombine(hash, data4);
		return hash;
	}
};

inline ostream& operator<<(ostream& s, const Gleam::Reflection::Attribute::Guid& guid)
{
	ios_base::fmtflags f(s.flags());
    const auto& bytes = guid.mBytes;
	s << hex << setfill('0')
		<< setw(2) << (int)bytes[0]
		<< setw(2) << (int)bytes[1]
		<< setw(2) << (int)bytes[2]
		<< setw(2) << (int)bytes[3]
		<< "-"
		<< setw(2) << (int)bytes[4]
		<< setw(2) << (int)bytes[5]
		<< "-"
		<< setw(2) << (int)bytes[6]
		<< setw(2) << (int)bytes[7]
		<< "-"
		<< setw(2) << (int)bytes[8]
		<< setw(2) << (int)bytes[9]
		<< "-"
		<< setw(2) << (int)bytes[10]
		<< setw(2) << (int)bytes[11]
		<< setw(2) << (int)bytes[12]
		<< setw(2) << (int)bytes[13]
		<< setw(2) << (int)bytes[14]
		<< setw(2) << (int)bytes[15];
	s.flags(f);
	return s;
}

} // namespace std
