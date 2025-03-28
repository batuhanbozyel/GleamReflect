#include "ReflectionContext.h"

#include <clang/Tooling/Tooling.h>

using namespace Gleam;

ReflectionContext::ReflectionContext(const std::string_view name, const std::string& qualifiedName)
    : mName(name)
    , mQualifiedName(qualifiedName)
{
    
}

void ReflectionContext::ForwardDecls(std::stringstream& ss) const
{
	for (const auto& context : mContexts)
	{
		context.ForwardDecls(ss);
	}

	if (mEnums.empty() && mClasses.empty())
	{
		return;
	}

	ss << "namespace " << mQualifiedName << " {\n";
	for (const auto& [guid, enumDesc] : mEnums)
	{
		ss << "enum class " << enumDesc.ResolveName() << ";\n";
	}
	ss << "\n";
	for (const auto& [guid, classDesc] : mClasses)
	{
		ss << "class " << classDesc.ResolveName() << ";\n";
	}
	ss << "}\n";
}

void ReflectionContext::EmplaceContext(const ReflectionContext& context)
{
    mContexts.emplace_back(context);
}

const Reflection::ArrayDescription* ReflectionContext::RegisterArray(const Reflection::ArrayDescription& arrayDesc)
{
    uint32_t hash = static_cast<uint32_t>(mArrays.size());
    auto& desc = mArrays.emplace_back(arrayDesc);
    desc.mTypeHash = hash;
    return &desc;
}

const Reflection::ClassDescription* ReflectionContext::RegisterClass(const Reflection::ClassDescription& classDesc)
{
    return &mClasses.emplace_hint(mClasses.end(), classDesc.Guid(), classDesc)->second;
}

const Reflection::EnumDescription* ReflectionContext::RegisterEnum(const Reflection::EnumDescription& enumDesc)
{
    return &mEnums.emplace_hint(mEnums.end(), enumDesc.Guid(), enumDesc)->second;
}

const Reflection::ClassDescription* ReflectionContext::GetClass(const Reflection::Attribute::Guid& guid) const
{
    auto it = mClasses.find(guid);
    if (it == mClasses.end())
    {
        for (const auto& context : mContexts)
        {
            auto classDesc = context.GetClass(guid);
            if (classDesc)
            {
                return classDesc;
            }
        }
        return nullptr;
    }
    return &it->second;
}

const Reflection::EnumDescription* ReflectionContext::GetEnum(const Reflection::Attribute::Guid& guid) const
{
    auto it = mEnums.find(guid);
    if (it == mEnums.end())
    {
        for (const auto& context : mContexts)
        {
            auto enumDesc = context.GetEnum(guid);
            if (enumDesc)
            {
                return enumDesc;
            }
        }
        return nullptr;
    }
    return &it->second;
}

const std::string_view ReflectionContext::Name() const
{
    return mName;
}

const std::string_view ReflectionContext::QualifiedName() const
{
    return mQualifiedName;
}
