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
class ReflectionLinker;
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

class PrimitiveDescription
{
public:

	constexpr PrimitiveDescription() = default;
	
	constexpr PrimitiveDescription(PrimitiveType type)
		: mType(type)
	{
		
	}
	
	constexpr auto Type() const
	{
		return mType;
	}

	constexpr auto TypeHash() const
	{
		return static_cast<uint32_t>(mType);
	}

	constexpr auto GetSize() const
	{
		switch (mType)
		{
			case PrimitiveType::Bool:   return sizeof(bool);
			case PrimitiveType::WChar:  return sizeof(wchar_t);
			case PrimitiveType::Char:   return sizeof(char);
			case PrimitiveType::Int8:   return sizeof(int8_t);
			case PrimitiveType::Int16:  return sizeof(int16_t);
			case PrimitiveType::Int32:  return sizeof(int32_t);
			case PrimitiveType::Int64:  return sizeof(int64_t);
			case PrimitiveType::UInt8:  return sizeof(uint8_t);
			case PrimitiveType::UInt16: return sizeof(uint16_t);
			case PrimitiveType::UInt32: return sizeof(uint32_t);
			case PrimitiveType::UInt64: return sizeof(uint64_t);
			case PrimitiveType::Float:  return sizeof(float);
			case PrimitiveType::Double: return sizeof(double);
			default:                    return (size_t)0ull;
		}
	}
	
	constexpr const auto ResolveName() const
	{
		switch (mType)
		{
			case PrimitiveType::Bool:   return "bool";
			case PrimitiveType::WChar:  return "wchar_t";
			case PrimitiveType::Char:   return "char";
			case PrimitiveType::Int8:   return "int8_t";
			case PrimitiveType::Int16:  return "int16_t";
			case PrimitiveType::Int32:  return "int32_t";
			case PrimitiveType::Int64:  return "int64_t";
			case PrimitiveType::UInt8:  return "uint8_t";
			case PrimitiveType::UInt16: return "uint16_t";
			case PrimitiveType::UInt32: return "uint32_t";
			case PrimitiveType::UInt64: return "uint64_t";
			case PrimitiveType::Float:  return "float";
			case PrimitiveType::Double: return "double";
			case PrimitiveType::Void:   return "void";
			default:                    return "";
		}
	}

private:

	PrimitiveType mType = PrimitiveType::Invalid;
};

class TemplateParameterDescription
{
public:

	TemplateParameterDescription() = default;

	TemplateParameterDescription(MetaType type, uint32_t hash)
		: mType(type)
		, mHash(hash)
	{

	}

	MetaType GetType() const
	{
		return mType;
	}

	uint32_t TypeHash() const
	{
		return mHash;
	}

private:

	uint32_t mHash = 0;
	MetaType mType = MetaType::Invalid;
};

class MetaDescription
{
	friend class Gleam::ReflectionParser;
	friend class Gleam::ReflectionContext;
	friend class Gleam::ReflectionLinker;
public:

	MetaDescription() = default;
    
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

	template<const auto& Tag>
	bool HasAttribute() const
	{
		return HasAttribute<std::remove_cvref_t<decltype(Tag)>>();
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

	FieldDescription() = default;
    
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

	EnumCaseDescription() = default;
    
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
	friend class Gleam::ReflectionLinker;
public:

	EnumDescription() = default;
    
	EnumDescription(const MetaDescription& meta, size_t size, PrimitiveType underlyingType, const BufferView& cases)
		: MetaDescription(meta)
		, mSize(size)
		, mCases(cases)
		, mUnderlyingType(underlyingType)
    {
        
    }
    
    size_t GetSize() const
    {
        return mSize;
    }

    PrimitiveType UnderlyingType() const
    {
        return mUnderlyingType;
    }
    
    auto Cases() const
    {
        const auto ptr = IDatabase::GetInstance()->GetObject<EnumCaseDescription>(mCases);
        return DenseArrayView{ ptr, mCases.size / sizeof(EnumCaseDescription) };
    }
    
private:
    
    size_t mSize = 0;
    BufferView mCases = {};
    PrimitiveType mUnderlyingType = PrimitiveType::Invalid;
};

class BaseClassView
{
public:

	class Iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ClassDescription;
		using difference_type = std::ptrdiff_t;
		using pointer = const ClassDescription*;
		using reference = const ClassDescription&;

		explicit Iterator(const uint32_t* hash)
			: mHash(hash)
		{

		}

		reference operator*() const
		{
			return *IDatabase::GetInstance()->GetClass(*mHash);
		}

		pointer operator->() const
		{
			return IDatabase::GetInstance()->GetClass(*mHash);
		}

		Iterator& operator++()
		{
			++mHash;
			return *this;
		}

		bool operator==(const Iterator& other) const
		{
			return mHash == other.mHash;
		}

		bool operator!=(const Iterator& other) const
		{
			return mHash != other.mHash;
		}

	private:

		const uint32_t* mHash = nullptr;
	};

	BaseClassView(const uint32_t* hashes, size_t size)
		: mHashes(hashes)
		, mSize(size)
	{

	}

	const ClassDescription& operator[](size_t pos) const
	{
		return *IDatabase::GetInstance()->GetClass(mHashes[pos]);
	}

	size_t size() const
	{
		return mSize;
	}

	bool empty() const
	{
		return mSize == 0;
	}

	Iterator begin() const
	{
		return Iterator(mHashes);
	}

	Iterator end() const
	{
		return Iterator(mHashes + mSize);
	}

private:

	const uint32_t* mHashes = nullptr;
	size_t mSize = 0;
};

class ClassDescription : public MetaDescription
{
	friend class Gleam::ReflectionParser;
	friend class Gleam::ReflectionContext;
	friend class Gleam::ReflectionLinker;
public:
    
	ClassDescription() = default;

    ClassDescription(const MetaDescription& meta, size_t size, const BufferView& fields, const BufferView& bases, const BufferView& templateParams)
		: MetaDescription(meta)
		, mSize(size)
		, mFields(fields)
		, mBaseClasses(bases)
		, mTemplateParams(templateParams)
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
		return BaseClassView{ ptr, mBaseClasses.size / sizeof(uint32_t) };
	}

	auto ResolveTemplateParameters() const
	{
		const auto ptr = IDatabase::GetInstance()->GetObject<TemplateParameterDescription>(mTemplateParams);
		return DenseArrayView{ ptr, mTemplateParams.size / sizeof(TemplateParameterDescription) };
	}

	bool IsTemplate() const
	{
		return mTemplateParams.size > 0;
	}
    
    size_t GetSize() const
    {
        return mSize;
    }
    
private:
    
    size_t mSize = 0;
    BufferView mFields = {};
    BufferView mBaseClasses = {};
	BufferView mTemplateParams = {};
};

class ArrayDescription
{
public:

	ArrayDescription() = default;
    
    ArrayDescription(MetaType elementType, uint32_t elementHash, size_t size)
        : mElementType(elementType)
        , mElementHash(elementHash)
        , mSize(size)
    {
        
    }

    size_t GetSize() const
    {
        return mSize;
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
    uint32_t mElementHash = 0;
    MetaType mElementType = MetaType::Invalid;
};

} // namespace Gleam::Reflection
