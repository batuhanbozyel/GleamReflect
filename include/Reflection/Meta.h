#pragma once
#include "Attribute.h"
#include "IDatabase.h"
#include "Container/SparseArray.h"

#include <string_view>
#include <cstdint>
#include <cassert>
#include <variant>

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
    
    MetaDescription(const BufferView& name,
                    const BufferView& qualifiedName,
                    const BufferView& attributes,
                    const Attribute::Guid& guid,
                    uint32_t typeHash)
        : mName(name)
        , mQualifiedName(qualifiedName)
        , mGuid(guid)
        , mTypeHash(typeHash)
		, mAttributes(attributes)
    {
        
    }
    
    uint32_t TypeHash() const
    {
        return mTypeHash;
    }
    
    const Attribute::Guid& Guid() const
    {
        return mGuid;
    }
    
    const auto ResolveName() const
    {
		const auto str = IDatabase::GetInstance()->GetString(mName);
		assert(str != nullptr && "Name not found in the database");
		return std::string_view{ str, mName.size };
    }
    
    const auto ResolveQualifiedName() const
    {
		const auto str = IDatabase::GetInstance()->GetString(mQualifiedName);
		assert(str != nullptr && "Qualified name not found in the database");
		return std::string_view{ str, mQualifiedName.size };
    }
    
	template<AttributeType Attrib>
	bool HasAttribute() const
	{
		const auto attribs = IDatabase::GetInstance()->GetObject<uint32_t>(mAttributes);
		const auto numAttribs = mAttributes.size / sizeof(uint32_t);

        for (uint32_t i = 0; i < numAttribs; ++i)
        {
			if (attribs[i] == Attrib::description.hash)
			{
				return true;
			}
        }
        return false;
	}

	template<AttributeType Attrib>
	const Attrib* GetAttribute() const
	{
		const auto attribs = IDatabase::GetInstance()->GetObject<uint32_t>(mAttributes);
		const auto numAttribs = mAttributes.size / sizeof(uint32_t);

		for (uint32_t i = 0; i < numAttribs; ++i)
		{
			if (attribs[i] == Attrib::description.hash)
			{
				const auto view = IDatabase::GetInstance()->GetObject<BufferView>(
                {
					.offset = mAttributes.offset + mAttributes.size + i * sizeof(BufferView),
					.size = sizeof(BufferView)
			    });
                assert(view != nullptr && "Attribute view not found in the database");

                const auto attrib = IDatabase::GetInstance()->GetObject<Attrib>(*view);
				assert(attrib != nullptr && "Attribute not found in the database");
                return attrib;
			}
		}
        return nullptr;
	}
    
private:
    
    uint32_t mTypeHash = 0;
    BufferView mAttributes = {};
    BufferView mName = {};
	BufferView mQualifiedName = {};
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
    
    size_t GetOffset() const
    {
        return mOffset;
    }
    
    size_t GetSize() const
    {
        return mSize;
    }
    
    MetaType GetType() const
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
    
    int64_t Value() const
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
    
    size_t GetSize() const
    {
        return mSize;
    }
    
    auto Cases() const
    {
        const auto ptr = IDatabase::GetInstance()->GetObject<EnumCaseDescription>(mCases);
        return DenseArrayView{ ptr, mCases.size / sizeof(EnumCaseDescription) };
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
    
    auto ResolveFields() const
    {
		const auto ptr = IDatabase::GetInstance()->GetObject<FieldDescription>(mFields);
		return DenseArrayView{ ptr, mFields.size / sizeof(FieldDescription) };
    }
    
    auto ResolveBaseClasses() const
    {
		const auto ptr = IDatabase::GetInstance()->GetObject<uint32_t>(mBaseClasses);
		auto indices = DenseArrayView{ ptr, mBaseClasses.size / sizeof(BufferView) };
        auto classes = IDatabase::GetInstance()->GetClasses();
        return SparseArrayView{ classes.data(), indices };
    }
    
    size_t GetSize() const
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
    
    size_t GetSize() const
    {
        return mSize;
    }
    
    size_t GetStride() const
    {
        return mStride;
    }
    
    uint32_t ElementHash() const
    {
        return mElementHash;
    }
    
    MetaType ElementType() const
    {
        return mElementType;
    }
    
private:
    
    size_t mSize = 0;
    size_t mStride = 0;
    uint32_t mElementHash = 0;
    MetaType mElementType = MetaType::Invalid;
    
};

} // namespace Gleam::Reflection
