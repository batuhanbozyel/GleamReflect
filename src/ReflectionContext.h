#pragma once
#include "Reflection/Meta.h"

#include <span>
#include <mutex>
#include <vector>
#include <string>
#include <sstream>
#include <string_view>
#include <unordered_map>

#define InvalidMetaIndex ~0u

namespace Gleam {

class ReflectionParser;

struct EnumHandle
{
	uint32_t index = InvalidMetaIndex;

	operator uint32_t() const
	{
		return index;
	}
};
static_assert(sizeof(EnumHandle) == sizeof(uint32_t), "EnumHandle must be the same size as uint32_t");

struct ClassHandle
{
	uint32_t index = InvalidMetaIndex;

	operator uint32_t() const
	{
		return index;
	}
};
static_assert(sizeof(ClassHandle) == sizeof(uint32_t), "ClassHandle must be the same size as uint32_t");

struct ArrayHandle
{
	uint32_t index = InvalidMetaIndex;

	operator uint32_t() const
	{
		return index;
	}
};
static_assert(sizeof(ArrayHandle) == sizeof(uint32_t), "ArrayHandle must be the same size as uint32_t");

class ReflectionContext
{
    using EnumMap = std::unordered_map<Reflection::Attribute::Guid, EnumHandle>;
    using ClassMap = std::unordered_map<Reflection::Attribute::Guid, std::vector<ClassHandle>>;

    using EnumList = std::vector<Reflection::EnumDescription>;
    using ClassList = std::vector<Reflection::ClassDescription>;
    using ArrayList = std::vector<Reflection::ArrayDescription>;
	using ClassTemplateDeclList = std::vector<std::string>;
	using TypeHashMap = std::unordered_map<uint32_t, uint32_t>;
public:
    
    explicit ReflectionContext(const ReflectionParser* parser, const std::string& name, const std::string& qualifiedName);
	~ReflectionContext();
    
	void GenerateForwardDecls(std::stringstream& ss) const;
    void GenerateMetaDescs(std::stringstream& ss) const;

    ReflectionContext* EmplaceContext(const std::string& qualifiedName);
    
    EnumHandle RegisterEnum(const Reflection::EnumDescription& enumDesc);
    ArrayHandle RegisterArray(const Reflection::ArrayDescription& arrayDesc);
    ClassHandle RegisterClass(const Reflection::ClassDescription& classDesc, const std::string& templateDecl);
    
    EnumHandle GetEnumHandle(uint32_t typeHash) const;
	EnumHandle GetEnumHandle(const Reflection::Attribute::Guid& guid) const;

	ClassHandle GetClassHandle(uint32_t typeHash) const;
	ClassHandle GetRegisteredClassInstance(const std::string_view name) const;
    std::span<const ClassHandle> GetClassHandles(const Reflection::Attribute::Guid& guid) const;
    
	bool Empty() const;
	bool Contains(const Reflection::Attribute::Guid& guid) const;
    
    const std::string_view Name() const;
    const std::string_view QualifiedName() const;

	const Reflection::EnumDescription& GetEnum(EnumHandle handle) const;
	const Reflection::EnumDescription& GetEnum(uint32_t typeHash) const;
	const Reflection::ClassDescription& GetClass(ClassHandle handle) const;
	const Reflection::ClassDescription& GetClass(uint32_t typeHash) const;
	const Reflection::ArrayDescription& GetArray(ArrayHandle handle) const;

	static const EnumList& GetEnums()
	{
		return mEnums;
	}

	static const ClassList& GetClasses()
	{
		return mClasses;
	}

	static const ArrayList& GetArrays()
	{
		return mArrays;
	}
    
private:

	mutable std::mutex mContextMutex;
	mutable std::mutex mEnumMutex;
	mutable std::mutex mArrayMutex;
	mutable std::mutex mClassMutex;

    EnumMap mGuidToEnum;
    ClassMap mGuidToClass;
    
    std::string mName;
    std::string mQualifiedName;
    std::vector<ReflectionContext*> mContexts;
	const ReflectionParser* mParser = nullptr;
    
	static inline EnumList mEnums = {};
	static inline ClassList mClasses = {};
	static inline ArrayList mArrays = {};
	static inline TypeHashMap mTypeHashMap = {};
	static inline ClassTemplateDeclList mClassTemplateDecls = {};
};

} // namespace Gleam
