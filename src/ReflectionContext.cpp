#include "ReflectionContext.h"

using namespace Gleam;

ReflectionContext::ReflectionContext(const std::string_view name, const std::string& qualifiedName)
    : mName(name)
    , mQualifiedName(qualifiedName)
{
    
}

void ReflectionContext::GenerateForwardDecls(std::stringstream& ss) const
{
	if (Empty())
	{
		return;
	}

    if (mName.length() > 0)
    {
        ss << "namespace " << mName << " {\n";
    }
    
    // Forward declarations
    {
        for (const auto& [guid, handle] : mGuidToEnum)
        {
            const auto& enumDesc = mEnums[handle];
            ss << "enum class " << enumDesc.ResolveName() << ";\n";
        }
        ss << "\n";
        for (const auto& [guid, handle] : mGuidToClass)
        {
            const auto& classDesc = mClasses[handle];
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

void ReflectionContext::GenerateMetaDescs(std::stringstream& ss) const
{
	if (Empty())
	{
		return;
	}

	for (const auto& [guid, handle] : mGuidToEnum)
	{
		const auto& enumDesc = mEnums[handle];
		ss << "template<>\n";
		ss << "inline const EnumDescription& GetEnum<" << mQualifiedName << "::" << enumDesc.ResolveName() << ">()\n";
		ss << "{\n";
		ss << "\tstatic const auto enums = IDatabase::GetInstance()->GetEnums();\n";
		ss << "\treturn enums[" << handle << "]; \n";
		ss << "}\n\n";
	}

    for (const auto& [guid, handle] : mGuidToClass)
    {
        const auto& classDesc = mClasses[handle];
        ss << "template<>\n";
        ss << "inline const ClassDescription& GetClass<" << mQualifiedName << "::" << classDesc.ResolveName() << ">()\n";
        ss << "{\n";
        ss << "\tstatic const auto classes = IDatabase::GetInstance()->GetClasses();\n";
        ss << "\treturn classes[" << handle << "]; \n";
        ss << "}\n\n";
    }

    for (const auto& context : mContexts)
    {
        ss << "\n";
        context.GenerateMetaDescs(ss);
    }
}

ReflectionContext& ReflectionContext::EmplaceContext(const std::string_view name, const std::string& qualifiedName)
{
	auto it = std::find_if(mContexts.begin(), mContexts.end(), [&](const ReflectionContext& ctx)
	{
		return ctx.QualifiedName() == qualifiedName;
	});
	if (it != mContexts.end())
	{
		return *it;
	}
	return mContexts.emplace_back(name, qualifiedName);
}

ArrayHandle ReflectionContext::RegisterArray(const Reflection::ArrayDescription& arrayDesc)
{
    uint32_t index = static_cast<uint32_t>(mArrays.size());
    mArrays.emplace_back(arrayDesc);
    return ArrayHandle(index);
}

ClassHandle ReflectionContext::RegisterClass(const Reflection::ClassDescription& classDesc)
{
    auto it = mGuidToClass.find(classDesc.Guid());
    if (it != mGuidToClass.end())
    {
        // ASSERT duplicate guid
        return {};
    }
    
    uint32_t index = static_cast<uint32_t>(mClasses.size());
    mGuidToClass.emplace_hint(mGuidToClass.end(), classDesc.Guid(), index);
    mClasses.emplace_back(classDesc);
    return ClassHandle(index);
}

EnumHandle ReflectionContext::RegisterEnum(const Reflection::EnumDescription& enumDesc)
{
    auto it = mGuidToEnum.find(enumDesc.Guid());
    if (it != mGuidToEnum.end())
    {
        // ASSERT duplicate guid
        return {};
    }
    
    uint32_t index = static_cast<uint32_t>(mEnums.size());
    mGuidToEnum.emplace_hint(mGuidToEnum.end(), enumDesc.Guid(), index);
    mEnums.emplace_back(enumDesc);
    return EnumHandle(index);
}

ClassHandle ReflectionContext::GetClassHandle(const Reflection::Attribute::Guid& guid) const
{
    auto it = mGuidToClass.find(guid);
    if (it == mGuidToClass.end())
    {
        for (const auto& context : mContexts)
        {
            auto handle = context.GetClassHandle(guid);
            if (handle.index < mClasses.size())
            {
                return handle;
            }
        }
        return ClassHandle(InvalidMetaIndex);
    }
    return it->second;
}

EnumHandle ReflectionContext::GetEnumHandle(const Reflection::Attribute::Guid& guid) const
{
    auto it = mGuidToEnum.find(guid);
    if (it == mGuidToEnum.end())
    {
        for (const auto& context : mContexts)
        {
            auto handle = context.GetEnumHandle(guid);
            if (handle.index < mEnums.size())
            {
                return handle;
            }
        }
        return EnumHandle(InvalidMetaIndex);
    }
    return it->second;
}

bool ReflectionContext::Empty() const
{
	if ((mGuidToEnum.empty() && mGuidToClass.empty()) == false)
	{
		return false;
	}

	for (const auto& ctx : mContexts)
	{
		if (ctx.Empty() == false)
		{
			return false;
		}
	}
	return true;
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
