#pragma once
#include "Meta.h"

#include <filesystem>
#include <unordered_map>

namespace Gleam::Reflection {

struct BinaryHeader
{
    char magic[8];  // "GLEAMREF"
    uint32_t version;
    uint32_t classCount;
    uint32_t enumCount;
    size_t classTableOffset;
    size_t enumTableOffset;
};

struct BinaryBuffer
{
    void* data = nullptr;
    size_t size = 0;
    
    void Allocate(size_t newSize)
    {
        if (size == newSize || newSize == 0) { return; }
        if (data) { Free(); }
        
        size = newSize;
        data = ::operator new(newSize);
    }
    
    void Free()
    {
        if (data)
        {
            ::operator delete(data);
            data = nullptr;
            size = 0;
        }
    }
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
    
private:
    
    void BuildLookupTables(const BinaryHeader* header);
    
    std::unordered_map<uint32_t, size_t> mClassHashToOffsets;
    std::unordered_map<uint32_t, size_t> mEnumHashToOffsets;
    
    std::unordered_map<Attribute::Guid, size_t> mClassGuidToOffsets;
    std::unordered_map<Attribute::Guid, size_t> mEnumGuidToOffsets;
    
    BinaryBuffer mBuffer;
    
};

} // namespace Gleam::Reflection
