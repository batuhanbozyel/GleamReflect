#pragma once
#include "Serialization/BinaryBuffer.h"
#include "Container/DenseArray.h"
#include "Attribute.h"

#include <filesystem>
#include <unordered_map>

namespace Gleam::Reflection {

class ClassDescription;
class EnumDescription;

namespace Attribute {
struct Guid;
} // namespace Attribute

struct DatabaseHeader
{
    char magic[8];  // "GLEAMREF"
    uint32_t version;
    uint32_t classCount;
    uint32_t enumCount;
	uint32_t arrayCount;
    size_t classTableOffset;
    size_t enumTableOffset;
	size_t arrayTableOffset;
	size_t stringTableOffset;
};

class Database
{
public:
    
    bool Initialize(const std::filesystem::path& path);
    
    void Shutdown();
    
    const ClassDescription* GetClass(uint32_t hash) const;
    const ClassDescription* GetClass(const Attribute::Guid& guid) const;
    
    const EnumDescription* GetEnum(uint32_t hash) const;
    const EnumDescription* GetEnum(const Attribute::Guid& guid) const;

    DenseArrayView<ClassDescription> GetClasses() const
    {
		const auto header = static_cast<const DatabaseHeader*>(mBuffer.data);
        return DenseArrayView{ Utils::OffsetPointer<ClassDescription>(mBuffer.data, header->classTableOffset), header->classCount };
    }

	DenseArrayView<EnumDescription> GetEnums() const
	{
		const auto header = static_cast<const DatabaseHeader*>(mBuffer.data);
		return DenseArrayView{ Utils::OffsetPointer<EnumDescription>(mBuffer.data, header->enumTableOffset), header->enumCount };
	}

	template<typename T>
    const T* GetObject(const BufferView& view) const
    {
		auto offset = view.offset + sizeof(DatabaseHeader);
        if ((offset + view.size) > mBuffer.size)
        {
            return nullptr;
        }
        return Utils::OffsetPointer<T>(mBuffer.data, offset);
    }

	const char* GetString(const BufferView& view) const
	{
		const auto header = static_cast<const DatabaseHeader*>(mBuffer.data);
		auto offset = view.offset + header->stringTableOffset;
		if ((offset + view.size) > mBuffer.size)
		{
			return nullptr;
		}
		return Utils::OffsetPointer<char>(mBuffer.data, offset);
	}
    
private:
    
    void BuildLookupTables(const DatabaseHeader* header);
    
    std::unordered_map<uint32_t, size_t> mClassHashToOffsets;
    std::unordered_map<uint32_t, size_t> mEnumHashToOffsets;
    
    std::unordered_map<Attribute::Guid, size_t> mClassGuidToOffsets;
    std::unordered_map<Attribute::Guid, size_t> mEnumGuidToOffsets;
    
    BinaryBuffer mBuffer;
    
};
extern Database* gReflectionDatabase;

} // namespace Gleam::Reflection
