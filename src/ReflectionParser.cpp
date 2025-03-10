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
    const auto& recordAttribs = ParseAttributes(recordDecl);
    const auto& recordGuid = ExtractGuid(recordAttribs);
    assert(recordGuid != Reflection::Attribute::Guid::InvalidGuid() &&  "Record is missing GUID attribute");
    
    // Process fields
    for (const auto* field : recordDecl->fields())
    {
        const auto& attribs = ParseAttributes(field);
        const auto& guid = ExtractGuid(attribs);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Field is missing GUID attribute");
    }
    
    // Process functions
    for (const auto* method : recordDecl->methods())
    {
        const auto& attribs = ParseAttributes(method);
        const auto& guid = ExtractGuid(attribs);
        assert(guid != Reflection::Attribute::Guid::InvalidGuid() && "Function is missing GUID attribute");
    }
}

std::vector<AttributePair> ReflectionParser::ParseAttributes(const clang::Decl* decl)
{
    std::vector<AttributePair> attributes;
    for (const auto* attr : decl->attrs())
    {
        if (auto* annotateAttr = llvm::dyn_cast<clang::AnnotateAttr>(attr))
        {
            std::string annotation = annotateAttr->getAnnotation().str();
            if (annotation.find("GCLASS") == 0 ||
                annotation.find("GSTRUCT") == 0 ||
                annotation.find("GENUM") == 0 ||
                annotation.find("GITEM") == 0 ||
                annotation.find("GFIELD") == 0 ||
                annotation.find("GFUNCTION") == 0)
            {
                std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
                std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
                std::sregex_iterator end;
                
                for (; it != end; ++it)
                {
                    std::string attrName = (*it)[1].str();
                    std::string argsStr = (*it)[2].str();
                    attributes.emplace_back(AttributePair{
                        .description = Reflection::AttributeDescription(attrName.c_str()),
                        .arguments = argsStr });
                }
            }
        }
    }
    return attributes;
}

Reflection::Attribute::Guid ReflectionParser::ExtractGuid(const std::vector<AttributePair>& attributes)
{
    for (const auto& attr : attributes)
    {
        if (attr.description.hash == Reflection::Utils::HashString("Guid"))
        {
            std::regex guidRegex("\\{?([0-9a-fA-F]{8})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{4})-?([0-9a-fA-F]{12})\\}?");
            std::smatch matches;
            if (std::regex_search(attr.arguments, matches, guidRegex) && matches.size() == 6)
            {
                // Format the GUID in the standard format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
                std::string formattedGuid =
                    matches[1].str() + "-" +
                    matches[2].str() + "-" +
                    matches[3].str() + "-" +
                    matches[4].str() + "-" +
                    matches[5].str();
                
                return Reflection::Attribute::Guid(formattedGuid.c_str());
            }
            // No valid GUID format is found
            return Reflection::Attribute::Guid::InvalidGuid();
        }
    }
    return Reflection::Attribute::Guid::InvalidGuid();
}

} // namespace Gleam
