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
class ReflectionContext;
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
    
    MetaDescription(const std::string_view name,
                    const std::vector<IAttribute*>& attributes,
                    const Attribute::Guid& guid,
                    uint32_t typeHash)
        : mName(name)
        , mGuid(guid)
        , mTypeHash(typeHash)
		, mAttributes(attributes)
    {
        
    }
    
    virtual ~MetaDescription() = default;
    
    constexpr uint32_t TypeHash() const
    {
        return mTypeHash;
    }
    
    constexpr const Attribute::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const std::string_view ResolveName() const
    {
        return mName;
    }
    
	template<AttributeType Attrib>
	constexpr bool HasAttribute() const
	{
		for (const auto attrib : mAttributes)
		{
			if (attrib->GetDescription().hash == Attrib::description.hash)
			{
				return true;
			}
		}
        return false;
	}

	template<AttributeType Attrib>
	constexpr const Attrib* GetAttribute() const
	{
		for (const auto attrib : mAttributes)
		{
			if (attrib->GetDescription().hash == Attrib::description.hash)
			{
                return static_cast<const Attrib*>(attrib);
			}
		}
	}
    
private:
    
    uint32_t mTypeHash = 0;
    const std::string_view mName = "";
    Attribute::Guid mGuid = Attribute::Guid::InvalidGuid();
    std::vector<IAttribute*> mAttributes = {};
};

class FieldDescription : public MetaDescription
{
public:
    
	FieldDescription(const MetaDescription& meta, size_t offset, size_t size, MetaType type)
        : MetaDescription(meta)
		, mOffset(offset)
		, mSize(size)
		, mType(type)
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
    
    constexpr MetaType GetType() const
    {
        return mType;
    }
    
private:
    
    size_t mSize = 0;
    size_t mOffset = 0;
    MetaType mType = MetaType::Invalid;
};

class EnumCaseDescription : public MetaDescription
{
public:
    
	EnumCaseDescription(const MetaDescription& meta, int64_t value)
		: MetaDescription(meta)
		, mValue(value)
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
public:
    
	EnumDescription(const MetaDescription& meta, size_t size, const std::vector<EnumCaseDescription>& cases)
		: MetaDescription(meta)
		, mSize(size)
		, mCases(cases)
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
public:
    
    ClassDescription(const MetaDescription& meta, size_t size, const std::vector<FieldDescription>& fields, const std::vector<ClassDescription>& bases)
		: MetaDescription(meta)
		, mSize(size)
		, mFields(fields)
		, mBaseClasses(bases)
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
    
private:
    
    size_t mSize = 0;
    std::vector<FieldDescription> mFields = {};
    std::vector<ClassDescription> mBaseClasses = {};
};

class ArrayDescription
{
    friend class Gleam::ReflectionParser;
    friend class Gleam::ReflectionContext;
public:
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t GetStride() const
    {
        return mStride;
    }
    
    constexpr uint32_t TypeHash() const
    {
        return mTypeHash;
    }
    
    constexpr uint32_t ElementHash() const
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
    uint32_t mTypeHash = 0;
    uint32_t mElementHash = 0;
    MetaType mElementType = MetaType::Invalid;
    
};

} // namespace Gleam::Reflection
