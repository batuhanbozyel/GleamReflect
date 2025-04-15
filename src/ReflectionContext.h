#pragma once
#include "Reflection/Meta.h"

#include <string>
#include <vector>
#include <string_view>
#include <filesystem>
#include <unordered_map>

namespace clang {
    class ASTContext;
    class Decl;
    class DeclContext;
    class EnumDecl;
    class CXXRecordDecl;
    class FieldDecl;
}

namespace Gleam {

class ReflectionContext
{
    using EnumMap = std::unordered_map<Reflection::Attribute::Guid, Reflection::EnumDescription>;
    using ClassMap = std::unordered_map<Reflection::Attribute::Guid, Reflection::ClassDescription>;
public:
    
    explicit ReflectionContext(const std::string_view name, const std::string& qualifiedName);
    
	void ForwardDecls(std::stringstream& ss) const;
    void CodeGen(std::stringstream& ss) const;

    void EmplaceContext(const ReflectionContext& context);
    
    const Reflection::ArrayDescription* RegisterArray(const Reflection::ArrayDescription& arrayDesc);
    const Reflection::ClassDescription* RegisterClass(const Reflection::ClassDescription& classDesc);
    const Reflection::EnumDescription* RegisterEnum(const Reflection::EnumDescription& enumDesc);
    
    const Reflection::ClassDescription* GetClass(const Reflection::Attribute::Guid& guid) const;
    const Reflection::EnumDescription* GetEnum(const Reflection::Attribute::Guid& guid) const;
    
    const std::string_view Name() const;
    const std::string_view QualifiedName() const;
    
private:
    EnumMap mEnums;
    ClassMap mClasses;
    std::vector<Reflection::ArrayDescription> mArrays;
    
    std::string_view mName;
    std::string mQualifiedName;
    std::vector<ReflectionContext> mContexts;
};

} // namespace Gleam
