#include "ReflectionParser.h"
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <cassert>
#include <regex>

namespace Gleam {

ReflectionParser::ReflectionParser()
{
    
}

void ReflectionParser::ParseAST(clang::ASTContext& context)
{
    for (const auto* decl : context.getTranslationUnitDecl()->decls())
    {
        if (const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl))
        {
            if (enumDecl->hasAttr<clang::AnnotateAttr>())
            {
                HandleEnumDecl(enumDecl);
            }
        }
        else if (const auto* recordDecl = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
        {
            if (recordDecl->hasAttr<clang::AnnotateAttr>() && recordDecl->isCompleteDefinition())
            {
                HandleRecordDecl(recordDecl);
            }
        }
    }
}

void ReflectionParser::GenerateOutput(const std::string& outputDir)
{
    
}

void ReflectionParser::HandleEnumDecl(const clang::EnumDecl* enumDecl)
{
//    std::vector<Attribute> attributes = ParseAttributes(enumDecl);
//    std::string guid = ExtractGuid(attributes);
//    
//    if (guid.empty())
//    {
//        return; // Skip if no GUID found
//    }
//    
//    EnumInfo enumInfo(enumDecl->getNameAsString(),
//                      enumDecl->getQualifiedNameAsString(),
//                      guid);
//    enumInfo.attributes = attributes;
//    
//    // Process enum values
//    for (const auto* enumConstant : enumDecl->enumerators())
//    {
//        std::vector<Attribute> valueAttributes = ParseAttributes(enumConstant);
//        std::string valueGuid = ExtractGuid(valueAttributes);
//        
//        if (valueGuid.empty())
//        {
//            continue; // Skip if no GUID found
//        }
//        
//        int value = enumConstant->getInitVal().getExtValue();
//        EnumValueInfo valueInfo(enumConstant->getNameAsString(), value, valueGuid);
//        valueInfo.attributes = valueAttributes;
//        
//        enumInfo.values.push_back(valueInfo);
//    }
//    
//    database.AddEnum(enumInfo);
}

void ReflectionParser::HandleRecordDecl(const clang::CXXRecordDecl* recordDecl)
{
    auto* recordAnnotateAttr = recordDecl->getAttr<clang::AnnotateAttr>();
    std::string recordAnnotation = recordAnnotateAttr->getAnnotation().str();
    
    if (recordAnnotation.find("GCLASS") || recordAnnotation.find("GSTRUCT"))
    {
        const auto& recordAttribs = ParseAttributes(recordAnnotation);
        const auto& recordGuid = ExtractGuid(recordAttribs);
        assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
        
        // Process fields
        for (const auto* field : recordDecl->fields())
        {
            auto* annotateAttr = field->getAttr<clang::AnnotateAttr>();
            std::string annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFIELD"))
            {
                const auto& attribs = ParseAttributes(annotation);
                const auto& guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Field is missing GUID attribute");
            }
        }
        
        // Process functions
        for (const auto* method : recordDecl->methods())
        {
            auto* annotateAttr = method->getAttr<clang::AnnotateAttr>();
            std::string annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GFUNCTION"))
            {
                const auto& attribs = ParseAttributes(annotation);
                const auto& guid = ExtractGuid(attribs);
                assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Function is missing GUID attribute");
            }
        }
        
        Reflection::ClassDescription classDesc(recordDecl->getName(), recordGuid);
    }
}

std::vector<AttributePair> ReflectionParser::ParseAttributes(const std::string& annotation)
{
    std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
    std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
    std::sregex_iterator end;
 
    std::vector<AttributePair> attributes;
    for (; it != end; ++it)
    {
        std::string attrName = (*it)[1].str();
        std::string argsStr = (*it)[2].str();
        attributes.emplace_back(AttributePair{
            .description = Reflection::AttributeDescription(attrName.c_str()),
            .arguments = argsStr });
    }
    return attributes;
}

Reflection::Attribute::Guid ReflectionParser::ExtractGuid(const std::vector<AttributePair>& attributes)
{
    for (const auto& attr : attributes)
    {
        if (attr.description.hash == Reflection::Utils::HashString("Guid"))
        {
            return Reflection::Attribute::Guid(attr.arguments);
        }
    }
    return Reflection::Attribute::Guid::InvalidGuid();
}

} // namespace Gleam
