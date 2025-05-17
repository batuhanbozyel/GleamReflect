#pragma once
#include "IDatabase.h"
#include "Meta.h"

namespace Gleam::Reflection {

class Database : public IDatabase
{
public:

	virtual bool Initialize(const std::filesystem::path& path) override
	{
		sInstance = this;

		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (not file.is_open())
		{
			std::cerr << "Failed to open reflection data file: " << path << std::endl;
			return false;
		}

		size_t bufferSize = file.tellg();
		file.seekg(0);

		mBuffer.Allocate(bufferSize);
		file.read(static_cast<char*>(mBuffer.data), mBuffer.size);

		const auto header = static_cast<const DatabaseHeader*>(mBuffer.data);
		if (memcmp(header->magic, "GLEAMREF", sizeof(header->magic)) != 0)
		{
			std::cerr << "Invalid reflection data file format" << std::endl;
			mBuffer.Free();
			return false;
		}

		if (header->version != GLEAM_REFLECTION_VERSION)
		{
			std::cerr << "Unsupported reflection data file version: " << header->version << std::endl;
			mBuffer.Free();
			return false;
		}
		BuildLookupTables(header);
		return true;
	}

	virtual void Shutdown() override
	{
		mBuffer.Free();

		mClassHashToOffsets.clear();
		mClassGuidToOffsets.clear();

		mEnumHashToOffsets.clear();
		mEnumGuidToOffsets.clear();

		mArrayHashToOffsets.clear();
		sInstance = nullptr;
	}

private:
    
    void BuildLookupTables(const DatabaseHeader* header)
	{
		for (uint32_t i = 0; i < header->classCount; ++i)
		{
			auto offset = header->classTableOffset + i * sizeof(ClassDescription);
			const auto classDesc = Utils::OffsetPointer<ClassDescription>(mBuffer.data, offset);
			
			mClassGuidToOffsets[classDesc->Guid()] = offset;
			mClassHashToOffsets[classDesc->TypeHash()] = offset;
		}
		
		for (uint32_t i = 0; i < header->enumCount; ++i)
		{
			auto offset = header->enumTableOffset + i * sizeof(EnumDescription);
			const auto enumDesc = Utils::OffsetPointer<EnumDescription>(mBuffer.data, offset);
			
			mEnumGuidToOffsets[enumDesc->Guid()] = offset;
			mEnumHashToOffsets[enumDesc->TypeHash()] = offset;
		}

		for (uint32_t i = 0; i < header->arrayCount; ++i)
		{
			auto offset = header->arrayTableOffset + i * sizeof(ArrayDescription);
			const auto arrayDesc = Utils::OffsetPointer<ArrayDescription>(mBuffer.data, offset);

			mArrayHashToOffsets[arrayDesc->ElementHash()] = offset;
		}
	}
    
};

} // namespace Gleam::Reflection
