#include "ReflectionContext.h"
#include "Serialization/BinaryWriter.h"

#include <clang/Tooling/Tooling.h>

using namespace Gleam;

ReflectionContext::ReflectionContext(const std::string_view name, const std::string& qualifiedName)
    : mName(name)
    , mQualifiedName(qualifiedName)
{
    
}

void ReflectionContext::GenerateForwardDecls(std::stringstream& ss) const
{
	if (mGuidToEnum.empty() && mGuidToClass.empty() && mContexts.empty())
	{
		return;
	}

    if (mName.length() > 0)
    {
        ss << "namespace " << mName << " {\n";
    }
    
    // Forward declarations
    {
        for (const auto& [guid, enumDesc] : mGuidToEnum)
        {
            ss << "enum class " << enumDesc.ResolveName() << ";\n";
        }
        ss << "\n";
        for (const auto& [guid, classDesc] : mGuidToClass)
        {
            ss << "class " << classDesc.ResolveName() << ";\n";
        }
        
        for (const auto& context : mContexts)
        {
            ss << "\n";
            context.GenerateForwardDecls(ss);
        }
    }
    
    if (mName.length() > 0)
    {
        ss << "} // namespace " << mName << "\n\n";
    }
}

void ReflectionContext::GenerateClassDescs(std::stringstream& ss) const
{
    for (const auto& [guid, index] : mGuidToClass)
    {
        const auto& classDesc = mClasses[index];
        auto classOffset = index * sizeof(Reflection::ClassDescription);
        
        ss << "template<>\n";
        ss << "inline const ClassDescription& GetClass<" << mQualifiedName << "::" << classDesc.ResolveName() << ">()\n";
        ss << "{\n";
        ss << "\tstatic const auto desc = gReflectionDatabase->GetObject<ClassDescription>(\n";
        ss << "\t{\n";
        ss << "\t\t.offset = " << classOffset << ",\n";
        ss << "\t\t.size = sizeof(ClassDescription)\n";
        ss << "\t});\n";
        ss << "\tassert(desc != nullptr);\n";
        ss << "\treturn *desc;\n";
        ss << "}\n\n";
    }
    
    for (const auto& context : mContexts)
    {
        ss << "\n";
        context.GenerateClassDescs(ss, writer);
    }
}

void ReflectionContext::EmplaceContext(const ReflectionContext& context)
{
    mContexts.emplace_back(context);
}

uint32_t ReflectionContext::RegisterArray(const Reflection::ArrayDescription& arrayDesc)
{
    uint32_t hash = static_cast<uint32_t>(mArrays.size());
    auto& desc = mArrays.emplace_back(arrayDesc);
    desc.mTypeHash = hash;
    return hash;
}

uint32_t ReflectionContext::RegisterClass(const Reflection::ClassDescription& classDesc)
{
    auto it = mGuidToClass.find(classDesc.Guid());
    if (it != mGuidToClass.end())
    {
        return it->second;
    }
    
    uint32_t index = static_cast<uint32_t>(mClasses.size());
    mGuidToClass.emplace_hint(mGuidToClass.end(), classDesc.Guid(), index);
    mClasses.emplace_back(classDesc);
    return index;
}

uint32_t ReflectionContext::RegisterEnum(const Reflection::EnumDescription& enumDesc)
{
    auto it = mGuidToEnum.find(enumDesc.Guid());
    if (it != mGuidToEnum.end())
    {
        return it->second;
    }
    
    uint32_t index = static_cast<uint32_t>(mEnums.size());
    mGuidToEnum.emplace_hint(mGuidToEnum.end(), enumDesc.Guid(), index);
    mEnums.emplace_back(enumDesc);
    return index;
}

uint32_t ReflectionContext::GetClassIndex(const Reflection::Attribute::Guid& guid) const
{
    auto it = mGuidToClass.find(guid);
    if (it == mGuidToClass.end())
    {
        for (const auto& context : mContexts)
        {
            auto index = context.GetClassIndex(guid);
            if (index < mClasses.size())
            {
                return index;
            }
        }
        return InvalidMetaIndex;
    }
    return it->second;
}

uint32_t ReflectionContext::GetEnumIndex(const Reflection::Attribute::Guid& guid) const
{
    auto it = mGuidToEnum.find(guid);
    if (it == mGuidToEnum.end())
    {
        for (const auto& context : mContexts)
        {
            auto index = context.GetEnumIndex(guid);
            if (index < mEnums.size())
            {
                return index;
            }
        }
        return InvalidMetaIndex;
    }
    return it->second;
}

bool ReflectionContext::Contains(const Reflection::Attribute::Guid& guid) const
{
    if (mGuidToEnum.contains(guid))
    {
        return true;
    }
    
    if (mGuidToClass.contains(guid))
    {
        return true;
    }
    
    for (const auto& ctx : mContexts)
    {
        if (ctx.Contains(guid))
        {
            return true;
        }
    }
    return false;
}

const std::string_view ReflectionContext::Name() const
{
    return mName;
}

const std::string_view ReflectionContext::QualifiedName() const
{
    return mQualifiedName;
}
