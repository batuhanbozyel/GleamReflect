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

	bool InstanceOfSameType(const Reflection::ClassDescription& lhs, const Reflection::ClassDescription& rhs) const;
    
private:
    void ParseDecls(const clang::DeclContext::decl_range& decls);
    EnumHandle HandleEnumDecl(const clang::EnumDecl* enumDecl);
    ClassHandle HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
    ArrayHandle HandleArrayType(const clang::ConstantArrayType* arrayType);

	ReflectionContext& GetDeclReflectionContext(const clang::DeclContext* declContext);
	const clang::ClassTemplateSpecializationDecl* GetTemplateSpecilization(const clang::CXXRecordDecl* recordDecl) const;
    
    size_t BuiltinTypeSize(const clang::BuiltinType* type) const;
    uint32_t BuiltinTypeHash(const clang::BuiltinType* type) const;

    Reflection::BufferView ParseAttributes(const std::string& annotation);
    Reflection::Attribute::Guid ExtractGuid(const Reflection::BufferView& attributes) const;
	std::string ExtractHeaderPath(const clang::SourceLocation& loc, clang::ASTContext& context) const;
	std::string ExtractTemplateDeclaration(const clang::CXXRecordDecl* recordDecl) const;
	std::string ExtractTemplateParameter(const clang::NamedDecl* param, const clang::PrintingPolicy& policy) const;
	std::string ExtractTemplateDefinition(const std::vector<Reflection::TemplateParameterDescription>& parameters) const;
	std::vector<Reflection::TemplateParameterDescription> ExtractTemplateParameterDescriptions(const clang::CXXRecordDecl* recordDecl);

	template<typename T>
	const T* ResolveObject(const Reflection::BufferView& view) const
	{
		const auto& buffer = mObjectWriter.GetBuffer();
		if ((view.offset + view.size) > buffer.size)
		{
			return nullptr;
		}
		return Reflection::Utils::OffsetPointer<T>(buffer.data, view.offset);
	}

	std::string_view ResolveString(const Reflection::BufferView& view) const
	{
		const auto& buffer = mStringWriter.GetBuffer();
		if ((view.offset + view.size) > buffer.size)
		{
			return {};
		}
		return std::string_view(Reflection::Utils::OffsetPointer<char>(buffer.data, view.offset), view.size);
	}

	std::string_view QualifiedNameWithoutTemplateDeclaration(const std::string_view name) const;

private:

    ReflectionContext mContext;
	std::set<std::string> mHeaders;
	std::set<std::string> mTemplateHeaders;
	Reflection::BinaryWriter mStringWriter;
    Reflection::BinaryWriter mObjectWriter;
};

} // namespace Gleam
