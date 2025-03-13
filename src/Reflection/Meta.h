#pragma once
#include "Attribute.h"

#include <string_view>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <variant>
#include <vector>

namespace Gleam {
class ReflectionParser;
} // namespace Gleam

namespace Gleam::Reflection {

enum class MetaType
{
    Invalid,
    Primitive,
    Array,
    Class,
    Enum
};

enum class PrimitiveType
{
    Invalid,
    Bool,
    WChar,
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    Void,
    
    COUNT
};

class MetaDescription
{
public:
    
    constexpr MetaDescription(const std::string_view name, const Attribute::Guid& guid)
        : mName(name)
        , mGuid(guid)
    {
        
    }
    
    virtual ~MetaDescription() = default;
    
    constexpr const Attribute::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const std::string_view ResolveName() const
    {
        return mName;
    }
    
    //    template<AttributeType Attrib>
    //    constexpr bool HasAttribute() const
    //    {
    //        for (const auto& attrib : mAttributes)
    //        {
    //            if (attrib.description.hash == Attrib::description.hash)
    //            {
    //                return true;
    //            }
    //        }
    //        return false;
    //    }
    //
    //    template<AttributeType Attrib>
    //    constexpr Attrib GetAttribute() const
    //    {
    //        for (const auto& attrib : mAttributes)
    //        {
    //            if (attrib.description.hash == Attrib::description.hash)
    //            {
    //                return std::any_cast<Attrib>(attrib.value);
    //            }
    //        }
    //        return Attrib({});
    //    }
    
private:
    
    const std::string_view mName = "";
    Attribute::Guid mGuid = Attribute::Guid::InvalidGuid();
    //std::vector<AttributePair> mAttributes = {};
};

class FieldDescription : public MetaDescription
{
    friend class Gleam::ReflectionParser;
public:
    
    constexpr FieldDescription(const std::string_view name, const Attribute::Guid& guid)
        : MetaDescription(name, guid)
    {
        
    }
    
    constexpr size_t GetOffset() const
    {
        return mOffset;
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t GetTypeHash() const
    {
        return mTypeHash;
    }
    
    constexpr MetaType GetType() const
    {
        return mType;
    }
    
private:
    
    size_t mSize = 0;
    size_t mOffset = 0;
    size_t mTypeHash = 0;
    MetaType mType = MetaType::Invalid;
};

class EnumCaseDescription : public MetaDescription
{
    friend class Gleam::ReflectionParser;
public:
    
    constexpr EnumCaseDescription(const std::string_view name, const Attribute::Guid& guid)
        : MetaDescription(name, guid)
    {
        
    }
    
    constexpr int64_t Value() const
    {
        return mValue;
    }
    
private:
    
    int64_t mValue = 0;
};

class EnumDescription : public MetaDescription
{
    friend class Gleam::ReflectionParser;
public:
    
    constexpr EnumDescription(const std::string_view name, const Attribute::Guid& guid)
        : MetaDescription(name, guid)
    {
        
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr const std::vector<EnumCaseDescription>& Cases() const
    {
        return mCases;
    }
    
private:
    
    size_t mSize = 0;
    std::vector<EnumCaseDescription> mCases = {};
    
};

class ClassDescription : public MetaDescription
{
    friend class Gleam::ReflectionParser;
public:
    
    constexpr ClassDescription(const std::string_view name, const Attribute::Guid& guid)
        : MetaDescription(name, guid)
    {
        
    }
    
    constexpr const std::vector<FieldDescription>& ResolveFields() const
    {
        return mFields;
    }
    
    constexpr const std::vector<ClassDescription>& ResolveBaseClasses() const
    {
        return mBaseClasses;
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t ContainerHash() const
    {
        return mContainerHash;
    }
    
private:
    
    size_t mSize = 0;
    size_t mContainerHash = 0;
    std::vector<FieldDescription> mFields = {};
    std::vector<ClassDescription> mBaseClasses = {};
};

class ArrayDescription : public MetaDescription
{
    friend class Gleam::ReflectionParser;
public:
    
    constexpr ArrayDescription(const std::string_view name, const Attribute::Guid& guid)
        : MetaDescription(name, guid)
    {
        
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t GetStride() const
    {
        return mStride;
    }
    
    constexpr size_t ElementHash() const
    {
        return mElementHash;
    }
    
    constexpr MetaType ElementType() const
    {
        return mElementType;
    }
    
private:
    
    size_t mSize = 0;
    size_t mStride = 0;
    size_t mElementHash = 0;
    MetaType mElementType = MetaType::Invalid;
    
};

} // namespace Gleam::Reflection
