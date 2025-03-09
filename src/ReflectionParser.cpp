#include "ReflectionParser.h"
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>

namespace Gleam {

ReflectionParser::ReflectionParser()
{
    
}

bool ReflectionParser::ParseAST(clang::ASTContext& context)
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
    return true;
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
//    std::vector<Attribute> attributes = ParseAttributes(recordDecl);
//    std::string guid = ExtractGuid(attributes);
//    
//    if (guid.empty())
//    {
//        return; // Skip if no GUID found
//    }
//    
//    StructInfo structInfo(recordDecl->getNameAsString(),
//                         recordDecl->getQualifiedNameAsString(),
//                         guid);
//    structInfo.attributes = attributes;
//    
//    // Process fields
//    for (const auto* field : recordDecl->fields())
//    {
//        std::vector<Attribute> fieldAttributes = ParseAttributes(field);
//        std::string fieldGuid = ExtractGuid(fieldAttributes);
//        
//        if (fieldGuid.empty())
//        {
//            continue; // Skip if no GUID found
//        }
//        
//        FieldInfo fieldInfo(field->getNameAsString(),
//                            field->getType().getAsString(),
//                            fieldGuid);
//        fieldInfo.attributes = fieldAttributes;
//        
//        structInfo.fields.push_back(fieldInfo);
//    }
//    
//    database.AddStruct(structInfo);
}


} // namespace Gleam
