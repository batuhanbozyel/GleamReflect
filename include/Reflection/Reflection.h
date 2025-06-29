#pragma once
#include "Meta.h"
#include "IDatabase.h"
#include "TypeTraits.h"

namespace Gleam::Reflection {

template<typename T, std::enable_if_t<Traits::IsPrimitive<T>::value, bool> = true>
inline constexpr PrimitiveDescription GetPrimitive()
{
	if constexpr (Traits::IsSame<bool, T>::value)
        return PrimitiveDescription(PrimitiveType::Bool);
	else if constexpr (Traits::IsSame<wchar_t, T>::value)
        return PrimitiveDescription(PrimitiveType::WChar);
	else if constexpr (Traits::IsSame<char, T>::value)
        return PrimitiveDescription(PrimitiveType::Char);
	else if constexpr (Traits::IsSame<int8_t, T>::value)
        return PrimitiveDescription(PrimitiveType::Int8);
	else if constexpr (Traits::IsSame<int16_t, T>::value)
        return PrimitiveDescription(PrimitiveType::Int16);
	else if constexpr (Traits::IsSame<int32_t, T>::value)
        return PrimitiveDescription(PrimitiveType::Int32);
	else if constexpr (Traits::IsSame<int64_t, T>::value)
        return PrimitiveDescription(PrimitiveType::Int64);
	else if constexpr (Traits::IsSame<uint8_t, T>::value)
        return PrimitiveDescription(PrimitiveType::UInt8);
	else if constexpr (Traits::IsSame<uint16_t, T>::value)
        return PrimitiveDescription(PrimitiveType::UInt16);
	else if constexpr (Traits::IsSame<uint32_t, T>::value)
        return PrimitiveDescription(PrimitiveType::UInt32);
	else if constexpr (Traits::IsSame<uint64_t, T>::value)
        return PrimitiveDescription(PrimitiveType::UInt64);
	else if constexpr (Traits::IsSame<float, T>::value)
        return PrimitiveDescription(PrimitiveType::Float);
	else if constexpr (Traits::IsSame<double, T>::value)
        return PrimitiveDescription(PrimitiveType::Double);
	else if constexpr (Traits::IsSame<void, T>::value)
        return PrimitiveDescription(PrimitiveType::Void);
	else
        return PrimitiveDescription(PrimitiveType::Invalid);
}

inline constexpr PrimitiveType GetPrimitiveType(uint32_t hash)
{
    return static_cast<PrimitiveType>(hash);
}

inline constexpr PrimitiveDescription GetPrimitive(uint32_t hash)
{
	return PrimitiveDescription(GetPrimitiveType(hash));
}

template<typename T, std::enable_if_t<Traits::IsClass<T>::value, bool> = true>
inline const ClassDescription& GetClass()
{
	assert(false && "Class is not reflected");

	static ClassDescription invalidDesc;
	return invalidDesc;
}

inline const ClassDescription* GetClass(uint32_t hash)
{
    return IDatabase::GetInstance()->GetClass(hash);
}

inline const ClassDescription* GetClass(const char* name)
{
	auto hash = Utils::HashString(name);
	return GetClass(hash);
}

template<typename T, std::enable_if_t<Traits::IsEnum<T>::value, bool> = true>
inline const EnumDescription& GetEnum()
{
	assert(false && "Enum is not reflected");

	static EnumDescription invalidDesc;
	return invalidDesc;
}

inline const EnumDescription* GetEnum(uint32_t hash)
{
    return IDatabase::GetInstance()->GetEnum(hash);
}

inline const EnumDescription* GetEnum(const char* name)
{
	auto hash = Utils::HashString(name);
	return GetEnum(hash);
}

inline const ArrayDescription* GetArray(uint32_t hash)
{
    return IDatabase::GetInstance()->GetArray(hash);
}

template<typename T>
inline constexpr T& Get(void* ptr)
{
    return *static_cast<T*>(ptr);
}

template<typename T>
inline constexpr const T& Get(const void* ptr)
{
    return *static_cast<const T*>(ptr);
}

} // namespace Gleam::Reflection
