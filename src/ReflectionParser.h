#pragma once
#include "ReflectionContext.h"
#include "Serialization/BinaryWriter.h"

#include <clang/AST/DeclBase.h>

namespace Gleam {

class ReflectionParser
{
public:

    ReflectionParser();
    void ParseAST(clang::ASTContext& context);
    void GenerateOutput(const std::filesystem::path& outputDir);
    
private:
    void ParseDecls(ReflectionContext& context, const clang::DeclContext::decl_range& decls);
    EnumHandle HandleEnumDecl(ReflectionContext& context, const clang::EnumDecl* enumDecl);
    ClassHandle HandleRecordDecl(ReflectionContext& context, const clang::CXXRecordDecl* recordDecl);
    ArrayHandle HandleArrayType(ReflectionContext& context, const clang::ConstantArrayType* arrayType);
    
    size_t BuiltinTypeSize(const clang::BuiltinType* type) const;
    uint32_t BuiltinTypeHash(const clang::BuiltinType* type) const;
    
    std::vector<Reflection::IAttribute*> ParseAttributes(const std::string& annotation) const;
    Reflection::Attribute::Guid ExtractGuid(const std::vector<Reflection::IAttribute*>& attributes) const;

private:

    ReflectionContext mContext;
    Reflection::BinaryWriter mObjectWriter;
};

} // namespace Gleam
