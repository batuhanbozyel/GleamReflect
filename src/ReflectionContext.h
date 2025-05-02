#pragma once
#include "Reflection/Meta.h"

#include <string>
#include <vector>
#include <string_view>
#include <filesystem>
#include <unordered_map>

#define InvalidMetaIndex ~0u

namespace clang {
class ASTContext;
class Decl;
class DeclContext;
class EnumDecl;
class CXXRecordDecl;
class FieldDecl;
}

namespace Gleam {

class ReflectionParser;

class ReflectionContext
{
    friend class ReflectionParser;
    
    using MetaMap = std::unordered_map<Reflection::Attribute::Guid, uint32_t>;
    using EnumList = std::vector<Reflection::EnumDescription>;
    using ClassList = std::vector<Reflection::ClassDescription>;
    using ArrayList = std::vector<Reflection::ArrayDescription>;
public:
    
    explicit ReflectionContext(const std::string_view name, const std::string& qualifiedName);
    
	void GenerateForwardDecls(std::stringstream& ss) const;
    void GenerateClassDescs(std::stringstream& ss) const;

    void EmplaceContext(const ReflectionContext& context);
    
    uint32_t RegisterArray(const Reflection::ArrayDescription& arrayDesc);
    uint32_t RegisterClass(const Reflection::ClassDescription& classDesc);
    uint32_t RegisterEnum(const Reflection::EnumDescription& enumDesc);
    
    uint32_t GetClassIndex(const Reflection::Attribute::Guid& guid) const;
    uint32_t GetEnumIndex(const Reflection::Attribute::Guid& guid) const;
    
    bool Contains(const Reflection::Attribute::Guid& guid) const;
    
    const std::string_view Name() const;
    const std::string_view QualifiedName() const;
    
private:
    MetaMap mGuidToEnum;
    MetaMap mGuidToClass;
    
    std::string_view mName;
    std::string mQualifiedName;
    std::vector<ReflectionContext> mContexts;
    
    static inline EnumList mEnums;
    static inline ClassList mClasses;
    static inline ArrayList mArrays;
};

} // namespace Gleam
