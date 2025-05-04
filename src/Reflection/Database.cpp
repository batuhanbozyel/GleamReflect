#include "Database.h"
#include "Meta.h"

#include <fstream>
#include <iostream>
#include <string_view>

using namespace Gleam::Reflection;

bool Database::Initialize(const std::filesystem::path& path)
{
    gReflectionDatabase = this;
    
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
    
    BuildLookupTables(header);
    return true;
}

void Database::Shutdown()
{
    mBuffer.Free();
    
    mClassHashToOffsets.clear();
    mClassGuidToOffsets.clear();
    
    mEnumHashToOffsets.clear();
    mEnumGuidToOffsets.clear();
}

const ClassDescription* Database::GetClass(uint32_t hash) const
{
    auto it = mClassHashToOffsets.find(hash);
    if (it != mClassHashToOffsets.end())
    {
        return Utils::OffsetPointer<ClassDescription>(mBuffer.data, it->second);
    }
    return nullptr;
}

const ClassDescription* Database::GetClass(const Attribute::Guid& guid) const
{
    auto it = mClassGuidToOffsets.find(guid);
    if (it != mClassGuidToOffsets.end())
    {
        return Utils::OffsetPointer<ClassDescription>(mBuffer.data, it->second);
    }
    return nullptr;
}

const EnumDescription* Database::GetEnum(uint32_t hash) const
{
    auto it = mEnumHashToOffsets.find(hash);
    if (it != mEnumHashToOffsets.end())
    {
        return Utils::OffsetPointer<EnumDescription>(mBuffer.data, it->second);
    }
    return nullptr;
}

const EnumDescription* Database::GetEnum(const Attribute::Guid& guid) const
{
    auto it = mEnumGuidToOffsets.find(guid);
    if (it != mEnumGuidToOffsets.end())
    {
        return Utils::OffsetPointer<EnumDescription>(mBuffer.data, it->second);
    }
    return nullptr;
}

void Database::BuildLookupTables(const DatabaseHeader* header)
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
}
