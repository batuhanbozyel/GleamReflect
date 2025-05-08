#pragma once
#include "ReflectionContext.h"
#include "Serialization/BinaryWriter.h"

#include <clang/AST/DeclBase.h>

#include <filesystem>

namespace clang {
class ASTContext;
class CXXRecordDecl;
class EnumDecl;
class ConstantArrayType;
class BuiltinType;
} // namespace clang

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
    
    Reflection::BufferView ParseAttributes(const std::string& annotation);
    Reflection::Attribute::Guid ExtractGuid(const Reflection::BufferView& attributes) const;

private:

    ReflectionContext mContext;
    Reflection::BinaryWriter mObjectWriter;
};

} // namespace Gleam
