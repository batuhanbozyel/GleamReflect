#pragma once
#include "Attribute.h"
#include "Database.h"

#include <string_view>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <variant>
#include <span>

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
                    const BufferView& attributes,
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
		const auto attribs = gReflectionDatabase->GetObject<AttributeDescription>(mAttributes);
		const auto numAttribs = mAttributes.size / sizeof(AttributeDescription);

        for (uint32_t i = 0; i < numAttribs; ++i)
        {
			if (attribs[i].GetDescription().hash == Attrib::description.hash)
			{
				return true;
			}
        }
        return false;
	}

	template<AttributeType Attrib>
	constexpr const Attrib* GetAttribute() const
	{
		const auto attribs = gReflectionDatabase->GetObject<AttributeDescription>(mAttributes);
		const auto numAttribs = mAttributes.size / sizeof(AttributeDescription);

		for (uint32_t i = 0; i < numAttribs; ++i)
		{
			if (attribs[i].GetDescription().hash == Attrib::description.hash)
			{
				// TODO: retrieve attribute from the database
				// add size to the attribute description
                // refactor GLEAM_ATTRIBUTE to enforce POD types
			}
		}
        return nullptr;
	}
    
private:
    
    uint32_t mTypeHash = 0;
    BufferView mAttributes = {};
    const std::string_view mName = "";
    Attribute::Guid mGuid = Attribute::Guid::InvalidGuid();
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
    
	EnumDescription(const MetaDescription& meta, size_t size, const BufferView& cases)
		: MetaDescription(meta)
		, mSize(size)
		, mCases(cases)
    {
        
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr auto Cases() const
    {
        const auto ptr = gReflectionDatabase->GetObject<EnumCaseDescription>(mCases);
        return std::span{ ptr, mCases.size / sizeof(EnumCaseDescription) };
    }
    
private:
    
    size_t mSize = 0;
    BufferView mCases = {};
    
};

class ClassDescription : public MetaDescription
{
public:
    
    ClassDescription(const MetaDescription& meta, size_t size, const BufferView& fields, const BufferView& bases)
		: MetaDescription(meta)
		, mSize(size)
		, mFields(fields)
		, mBaseClasses(bases)
    {
        
    }
    
    constexpr auto ResolveFields() const
    {
		const auto ptr = gReflectionDatabase->GetObject<FieldDescription>(mFields);
		return std::span{ ptr, mFields.size / sizeof(FieldDescription) };
    }
    
    constexpr auto ResolveBaseClasses() const
    {
		const auto ptr = gReflectionDatabase->GetObject<ClassDescription>(mBaseClasses);
		return std::span{ ptr, mBaseClasses.size / sizeof(ClassDescription) };
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
private:
    
    size_t mSize = 0;
    BufferView mFields = {};
    BufferView mBaseClasses = {};
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
