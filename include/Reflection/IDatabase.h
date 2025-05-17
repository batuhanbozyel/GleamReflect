#pragma once
#include "Serialization/BinaryBuffer.h"
#include "Container/DenseArray.h"
#include "Attribute.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>
#include <unordered_map>

namespace Gleam::Reflection {

class ArrayDescription;
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

class IDatabase
{
public:

	static IDatabase* GetInstance()
	{
		assert(sInstance != nullptr && "Database instance is not initialized.");
		return sInstance;
	}

	IDatabase() = default;
	virtual ~IDatabase() = default;

	virtual bool Initialize(const std::filesystem::path& path) = 0;
	
	virtual void Shutdown() = 0;
    
    const ClassDescription* GetClass(uint32_t hash) const
	{
		auto it = mClassHashToOffsets.find(hash);
		if (it != mClassHashToOffsets.end())
		{
			return Utils::OffsetPointer<ClassDescription>(mBuffer.data, it->second);
		}
		return nullptr;
	}

    const ClassDescription* GetClass(const Attribute::Guid& guid) const
	{
		auto it = mClassGuidToOffsets.find(guid);
		if (it != mClassGuidToOffsets.end())
		{
			return Utils::OffsetPointer<ClassDescription>(mBuffer.data, it->second);
		}
		return nullptr;
	}
    
    const EnumDescription* GetEnum(uint32_t hash) const
	{
		auto it = mEnumHashToOffsets.find(hash);
		if (it != mEnumHashToOffsets.end())
		{
			return Utils::OffsetPointer<EnumDescription>(mBuffer.data, it->second);
		}
		return nullptr;
	}

    const EnumDescription* GetEnum(const Attribute::Guid& guid) const
	{
		auto it = mEnumGuidToOffsets.find(guid);
		if (it != mEnumGuidToOffsets.end())
		{
			return Utils::OffsetPointer<EnumDescription>(mBuffer.data, it->second);
		}
		return nullptr;
	}

	const ArrayDescription* GetArray(uint32_t hash) const
	{
		auto it = mArrayHashToOffsets.find(hash);
		if (it != mArrayHashToOffsets.end())
		{
			return Utils::OffsetPointer<ArrayDescription>(mBuffer.data, it->second);
		}
		return nullptr;
	}

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

	DenseArrayView<ArrayDescription> GetArrays() const
	{
		const auto header = static_cast<const DatabaseHeader*>(mBuffer.data);
		return DenseArrayView{ Utils::OffsetPointer<ArrayDescription>(mBuffer.data, header->arrayTableOffset), header->arrayCount };
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
    
protected:

	std::unordered_map<uint32_t, size_t> mArrayHashToOffsets;

    std::unordered_map<uint32_t, size_t> mClassHashToOffsets;
    std::unordered_map<uint32_t, size_t> mEnumHashToOffsets;
    
    std::unordered_map<Attribute::Guid, size_t> mClassGuidToOffsets;
    std::unordered_map<Attribute::Guid, size_t> mEnumGuidToOffsets;
    
    BinaryBuffer mBuffer;

	static inline IDatabase* sInstance = nullptr;
};

} // namespace Gleam::Reflection
