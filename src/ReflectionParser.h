#pragma once
#include "ReflectionContext.h"
#include "Serialization/BinaryWriter.h"

#include <clang/AST/DeclBase.h>

#include <set>
#include <filesystem>

namespace clang {
class ASTContext;
class CXXRecordDecl;
class ClassTemplateDecl;
class EnumDecl;
class ConstantArrayType;
class BuiltinType;
class TemplateParameterList;
class ClassTemplateSpecializationDecl;
} // namespace clang

namespace Gleam {

class ReflectionParser
{
public:

    ReflectionParser();
    void ParseAST(clang::ASTContext& context);
    void GenerateOutput(const std::string& moduleName,
						const std::filesystem::path& headerDir, 
						const std::filesystem::path& binaryDir);
    
private:
    void ParseDecls(const clang::DeclContext::decl_range& decls);
    EnumHandle HandleEnumDecl(const clang::EnumDecl* enumDecl);
    ClassHandle HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
    ArrayHandle HandleArrayType(const clang::ConstantArrayType* arrayType);

	ReflectionContext& GetDeclReflectionContext(const clang::DeclContext* declContext);
    
    size_t BuiltinTypeSize(const clang::BuiltinType* type) const;
    uint32_t BuiltinTypeHash(const clang::BuiltinType* type) const;

    Reflection::BufferView ParseAttributes(const std::string& annotation);
    Reflection::Attribute::Guid ExtractGuid(const Reflection::BufferView& attributes) const;
	std::string ExtractHeaderPath(const clang::SourceLocation& loc, clang::ASTContext& context) const;
	std::string ExtractTemplateDeclaration(const clang::ClassTemplateSpecializationDecl* specDecl) const;
	std::string ExtractTemplateParameter(const clang::NamedDecl* param, const clang::PrintingPolicy& policy) const;

private:

    ReflectionContext mContext;
	std::set<std::string> mHeaders;
	Reflection::BinaryWriter mStringWriter;
    Reflection::BinaryWriter mObjectWriter;
};

} // namespace Gleam
