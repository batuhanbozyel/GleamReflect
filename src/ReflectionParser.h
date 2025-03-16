#pragma once
#include "ReflectionContext.h"

#include <clang/AST/DeclBase.h>

namespace Gleam {

struct AttributePair
{
    Reflection::AttributeDescription description;
    std::string arguments;
};

class ReflectionParser
{
public:
    ReflectionParser();
    void ParseAST(clang::ASTContext& context);
    void GenerateOutput(const std::filesystem::path& outputDir);
    
private:
    void ParseDecls(ReflectionContext& context, const clang::DeclContext::decl_range& decls);
    const Reflection::EnumDescription* HandleEnumDecl(ReflectionContext& context, const clang::EnumDecl* enumDecl);
    const Reflection::ClassDescription* HandleRecordDecl(ReflectionContext& context, const clang::CXXRecordDecl* recordDecl);
    const Reflection::ArrayDescription* HandleArrayType(ReflectionContext& context, const clang::ConstantArrayType* arrayType);
    
    size_t BuiltinTypeSize(const clang::BuiltinType* type) const;
    uint32_t BuiltinTypeHash(const clang::BuiltinType* type) const;
    
    std::vector<AttributePair> ParseAttributes(const std::string& annotation) const;
    Reflection::Attribute::Guid ExtractGuid(const std::vector<AttributePair>& attributes) const;
private:
    ReflectionContext mContext;
};

} // namespace Gleam
