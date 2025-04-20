#pragma once
#include "Serialization/BinaryBuffer.h"

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
    size_t classTableOffset;
    size_t enumTableOffset;
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

	template<typename T>
    const T* GetObject(const BufferView& view) const
    {
        if ((view.offset + view.size) > mBuffer.size)
        {
            return nullptr;
        }
        return Utils::OffsetPointer<T>(mBuffer.data, view.offset);
    }
    
private:
    
    void BuildLookupTables(const DatabaseHeader* header);
    
    std::unordered_map<uint32_t, size_t> mClassHashToOffsets;
    std::unordered_map<uint32_t, size_t> mEnumHashToOffsets;
    
    std::unordered_map<Attribute::Guid, size_t> mClassGuidToOffsets;
    std::unordered_map<Attribute::Guid, size_t> mEnumGuidToOffsets;
    
    BinaryBuffer mBuffer;
    
};
extern Database* gReflectionDatabase = nullptr;

} // namespace Gleam::Reflection
