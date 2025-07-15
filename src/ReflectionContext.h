#pragma once
#include "Reflection/Meta.h"

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
    friend class ReflectionParser;
    
    using EnumMap = std::unordered_map<Reflection::Attribute::Guid, EnumHandle>;
    using ClassMap = std::unordered_map<Reflection::Attribute::Guid, ClassHandle>;

	using TemplateDeclList = std::vector<std::string>;
    using EnumList = std::vector<Reflection::EnumDescription>;
    using ClassList = std::vector<Reflection::ClassDescription>;
    using ArrayList = std::vector<Reflection::ArrayDescription>;
public:
    
    explicit ReflectionContext(const std::string& name, const std::string& qualifiedName);
    
	void GenerateForwardDecls(std::stringstream& ss) const;
    void GenerateMetaDescs(std::stringstream& ss) const;

    ReflectionContext& EmplaceContext(const std::string& qualifiedName);
    
    EnumHandle RegisterEnum(const Reflection::EnumDescription& enumDesc);
    ArrayHandle RegisterArray(const Reflection::ArrayDescription& arrayDesc);
    ClassHandle RegisterClass(const Reflection::ClassDescription& classDesc, const std::string& templateDecl);
    
    EnumHandle GetEnumHandle(const Reflection::Attribute::Guid& guid) const;
    ClassHandle GetClassHandle(const Reflection::Attribute::Guid& guid) const;
    
	bool Empty() const;
    bool Contains(const Reflection::Attribute::Guid& guid) const;
    
    const std::string_view Name() const;
    const std::string_view QualifiedName() const;
    
private:

    EnumMap mGuidToEnum;
    ClassMap mGuidToClass;
    
    std::string mName;
    std::string mQualifiedName;
    std::vector<ReflectionContext> mContexts;
    
	static inline EnumList mEnums = {};
	static inline ClassList mClasses = {};
	static inline ArrayList mArrays = {};
	static inline TemplateDeclList mTemplateDeclarations;
};

} // namespace Gleam
