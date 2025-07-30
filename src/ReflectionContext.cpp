#include "ReflectionContext.h"
#include "ReflectionParser.h"
#include "Reflection/Reflection.h"

using namespace Gleam;

static std::string GetParentPath(const std::string& qualifiedName)
{
	size_t lastSeparator = qualifiedName.rfind("::");
	if (lastSeparator == std::string::npos)
	{
		return "";
	}
	return qualifiedName.substr(0, lastSeparator);
}

static std::string GetContextName(const std::string& qualifiedName)
{
	size_t lastSeparator = qualifiedName.rfind("::");
	if (lastSeparator == std::string::npos)
	{
		return qualifiedName;
	}
	return qualifiedName.substr(lastSeparator + 2);
}

ReflectionContext::ReflectionContext(const ReflectionParser* parser, const std::string& name, const std::string& qualifiedName)
    : mName(name)
    , mQualifiedName(qualifiedName)
	, mParser(parser)
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
		if (mGuidToEnum.empty() == false)
		{
			for (const auto& [guid, handle] : mGuidToEnum)
			{
				const auto& enumDesc = mEnums[handle];
				ss << "enum class " << enumDesc.ResolveName() << ";\n";
			}
			ss << "\n";
		}
        
        for (const auto& [guid, classes] : mGuidToClass)
        {
			for (const auto handle : classes)
			{
				const auto& classDesc = mClasses[handle];
				if (classDesc.IsTemplate())
				{
					continue;
				}
				ss << "class " << classDesc.ResolveName() << ";\n";
			}
        }
        
        for (const auto& context : mContexts)
        {
			if (context.Empty() == false)
			{
				ss << "\n";
            	context.GenerateForwardDecls(ss);
			}
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
		ss << "inline const EnumDescription& GetEnumDesc<" << mQualifiedName << "::" << enumDesc.ResolveName() << ">()\n";
		ss << "{\n";
		ss << "\tstatic const auto enums = IDatabase::GetInstance()->GetEnums();\n";
		ss << "\treturn enums[" << handle << "];\n";
		ss << "}\n\n";
	}

    for (const auto& [guid, classes] : mGuidToClass)
    {
		for (const auto handle : classes)
		{
			const auto& classDesc = mClasses[handle];

			std::stringstream classNameSS;
			classNameSS << classDesc.ResolveName();

			if (classDesc.IsTemplate())
			{
				classNameSS << "<" << mClassTemplateDecls[handle] << ">";
			}

			ss << "template<>\n";
			ss << "inline const ClassDescription& GetClassDesc<" << mQualifiedName << "::" << classNameSS.str() << ">()\n";
			ss << "{\n";
			ss << "\tstatic const auto classes = IDatabase::GetInstance()->GetClasses();\n";
			ss << "\treturn classes[" << handle << "];\n";
			ss << "}\n\n";
		}
	}

    for (const auto& context : mContexts)
    {
		if (context.Empty() == false)
		{
			ss << "\n";
			context.GenerateMetaDescs(ss);
		}
    }
}

ReflectionContext& ReflectionContext::EmplaceContext(const std::string& qualifiedName)
{
	if (QualifiedName() == qualifiedName)
	{
		return *this;
	}

	std::string parentPath = GetParentPath(qualifiedName);
	std::string childName = GetContextName(qualifiedName);

	if (QualifiedName() == parentPath)
	{
		auto it = std::find_if(mContexts.begin(), mContexts.end(), [&](const ReflectionContext& ctx)
		{
			return ctx.Name() == childName;
		});

		if (it != mContexts.end())
		{
			return *it;
		}
		return mContexts.emplace_back(mParser, childName, qualifiedName);
	}

	ReflectionContext& parent = EmplaceContext(parentPath);
	return parent.EmplaceContext(qualifiedName);
}

ArrayHandle ReflectionContext::RegisterArray(const Reflection::ArrayDescription& arrayDesc)
{
    uint32_t index = static_cast<uint32_t>(mArrays.size());
    mArrays.emplace_back(arrayDesc);
    return ArrayHandle(index);
}

ClassHandle ReflectionContext::RegisterClass(const Reflection::ClassDescription& classDesc, const std::string& templateDecl)
{
    auto it = mGuidToClass.find(classDesc.Guid());
    if (it != mGuidToClass.end())
    {
		for (const auto handle : it->second)
		{
			if (mParser->InstanceOfSameType(mClasses[handle], classDesc))
			{
				goto REGISTER_CLASS;
			}
		}
        // ASSERT duplicate guid
        return {};
    }

REGISTER_CLASS:
    uint32_t index = static_cast<uint32_t>(mClasses.size());
	auto& guidToClass = mGuidToClass[classDesc.Guid()];
	guidToClass.emplace_back(index);
    mClasses.emplace_back(classDesc);
	mClassTemplateDecls.emplace_back(templateDecl);
	mTypeHashMap.emplace_hint(mTypeHashMap.end(), classDesc.TypeHash(), index);
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
	mTypeHashMap.emplace_hint(mTypeHashMap.end(), enumDesc.TypeHash(), index);
    mGuidToEnum.emplace_hint(mGuidToEnum.end(), enumDesc.Guid(), index);
    mEnums.emplace_back(enumDesc);
    return EnumHandle(index);
}

std::span<const ClassHandle> ReflectionContext::GetClassHandles(const Reflection::Attribute::Guid& guid) const
{
    auto it = mGuidToClass.find(guid);
    if (it == mGuidToClass.end())
    {
        for (const auto& context : mContexts)
        {
            const auto& handles = context.GetClassHandles(guid);
			if (handles.size() > 0)
			{
				return handles;
			}
        }
		return {};
    }
    return std::span{ it->second.data(), it->second.size() };
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
