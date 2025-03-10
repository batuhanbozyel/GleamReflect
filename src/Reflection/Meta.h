#pragma once
#include <string_view>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <variant>
#include <vector>

namespace Gleam::Reflection {

enum class FieldType
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

template<FieldType T>
struct FieldBase
{
    static constexpr FieldType type = T;
    
    size_t size = 0;
    size_t offset = 0;
};

struct NullField : FieldBase<FieldType::Invalid> {};

struct PrimitiveField : FieldBase<FieldType::Primitive>
{
    PrimitiveType primitive;
    
    explicit constexpr PrimitiveField(PrimitiveType primitive)
        : FieldBase(), primitive(primitive)
    {
        switch (primitive)
        {
            case PrimitiveType::Bool:
                size = sizeof(bool);
                break;
            case PrimitiveType::WChar:
                size = sizeof(wchar_t);
                break;
            case PrimitiveType::Char:
                size = sizeof(char);
                break;
            case PrimitiveType::Int8:
                size = sizeof(int8_t);
                break;
            case PrimitiveType::Int16:
                size = sizeof(int16_t);
                break;
            case PrimitiveType::Int32:
                size = sizeof(int32_t);
                break;
            case PrimitiveType::Int64:
                size = sizeof(int64_t);
                break;
            case PrimitiveType::UInt8:
                size = sizeof(uint8_t);
                break;
            case PrimitiveType::UInt16:
                size = sizeof(uint16_t);
                break;
            case PrimitiveType::UInt32:
                size = sizeof(uint32_t);
                break;
            case PrimitiveType::UInt64:
                size = sizeof(uint64_t);
                break;
            case PrimitiveType::Float:
                size = sizeof(float);
                break;
            case PrimitiveType::Double:
                size = sizeof(double);
                break;
            case PrimitiveType::Void:
                size = 0;
                break;
            default:
                assert(false && "Invalid primitive type!");
                break;
        }
    }
};

struct ArrayField : FieldBase<FieldType::Array>
{
    uint32_t hash;
    
    explicit constexpr ArrayField(uint32_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

struct ClassField : FieldBase<FieldType::Class>
{
    uint32_t hash;
    
    explicit constexpr ClassField(uint32_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

struct EnumField : FieldBase<FieldType::Enum>
{
    uint32_t hash;
    
    explicit constexpr EnumField(uint32_t hash)
        : FieldBase(), hash(hash)
    {
        
    }
};

// IMPORTANT: the order needs to match with FieldType enum items order
using Field = std::variant<NullField,
                           PrimitiveField,
                           ArrayField,
                           ClassField,
                           EnumField>;

class FieldDescription
{
public:
    
    template<typename T>
    constexpr const T& GetField() const
    {
        assert(T::type == mType && "Requested field type does not match!");
        return std::get<static_cast<uint32_t>(T::type)>(mField);
    }
    
    constexpr const Attribute::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const std::string_view ResolveName() const
    {
        return mName;
    }
    
    constexpr FieldType GetType() const
    {
        return mType;
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
    
    Field mField;
    FieldType mType;
    const std::string_view mName;
    Attribute::Guid mGuid;
    //std::vector<AttributePair> mAttributes;
};

class EnumDescription
{
public:
    
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
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
private:
    
    size_t mSize;
    const std::string_view mName;
    Attribute::Guid mGuid;
    //std::vector<AttributePair> mAttributes;
    
};

class ClassDescription
{
public:
    
    constexpr ClassDescription(const std::string_view name, const Attribute::Guid& guid)
        : mName(name)
        , mGuid(guid)
    {
        
    }

    constexpr const Attribute::Guid& Guid() const
    {
        return mGuid;
    }
    
    constexpr const std::string_view ResolveName() const
    {
        return mName;
    }
    
    constexpr const std::vector<FieldDescription>& ResolveFields() const
    {
        return mFields;
    }
    
    constexpr const std::vector<ClassDescription>& ResolveBaseClasses() const
    {
        return mBaseClasses;
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
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t ContainerHash() const
    {
        return mContainerHash;
    }
    
private:
    
    size_t mSize;
    const std::string_view mName;
    std::vector<FieldDescription> mFields;
    std::vector<ClassDescription> mBaseClasses;
    
    Attribute::Guid mGuid;
    //std::vector<AttributePair> mAttributes;
    
    uint32_t mContainerHash;
};

class ArrayDescription
{
public:
    
    ArrayDescription() = default;
    ArrayDescription(const ArrayDescription&) = default;
    ArrayDescription(const std::string_view name, FieldType type, uint32_t hash, size_t size, size_t stride)
        : mName(name), mType(type), mHash(hash), mSize(size), mStride(stride)
    {
        
    }
    
    constexpr const std::string_view ResolveName() const
    {
        return mName;
    }
    
    constexpr size_t GetSize() const
    {
        return mSize;
    }
    
    constexpr size_t GetStride() const
    {
        return mStride;
    }
    
    constexpr uint32_t ElementHash() const
    {
        return mHash;
    }
    
    constexpr FieldType ElementType() const
    {
        return mType;
    }
    
private:
    
    uint32_t mHash = 0;
    size_t mSize = 0;
    size_t mStride = 0;
    FieldType mType = FieldType::Invalid;
    const std::string_view mName = "";
    
};

} // namespace Gleam::Reflection
