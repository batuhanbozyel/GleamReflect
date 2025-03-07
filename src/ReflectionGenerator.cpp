#include "ReflectionGenerator.h"
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

ReflectionParser::ReflectionParser() {}

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
    std::vector<Attribute> attributes = ParseAttributes(enumDecl);
    std::string guid = ExtractGuid(attributes);
    
    if (guid.empty())
	{
        return; // Skip if no GUID found
    }
    
    EnumInfo enumInfo(enumDecl->getNameAsString(), 
                      enumDecl->getQualifiedNameAsString(),
                      guid);
    enumInfo.attributes = attributes;
    
    // Process enum values
    for (const auto* enumConstant : enumDecl->enumerators())
	{
        std::vector<Attribute> valueAttributes = ParseAttributes(enumConstant);
        std::string valueGuid = ExtractGuid(valueAttributes);
        
        if (valueGuid.empty())
		{
            continue; // Skip if no GUID found
        }
        
        int value = enumConstant->getInitVal().getExtValue();
        EnumValueInfo valueInfo(enumConstant->getNameAsString(), value, valueGuid);
        valueInfo.attributes = valueAttributes;
        
        enumInfo.values.push_back(valueInfo);
    }
    
    database.AddEnum(enumInfo);
}

void ReflectionParser::HandleRecordDecl(const clang::CXXRecordDecl* recordDecl)
{
    std::vector<Attribute> attributes = ParseAttributes(recordDecl);
    std::string guid = ExtractGuid(attributes);
    
    if (guid.empty())
	{
        return; // Skip if no GUID found
    }
    
    StructInfo structInfo(recordDecl->getNameAsString(), 
                         recordDecl->getQualifiedNameAsString(),
                         guid);
    structInfo.attributes = attributes;
    
    // Process fields
    for (const auto* field : recordDecl->fields())
	{
        std::vector<Attribute> fieldAttributes = ParseAttributes(field);
        std::string fieldGuid = ExtractGuid(fieldAttributes);
        
        if (fieldGuid.empty())
		{
            continue; // Skip if no GUID found
        }
        
        FieldInfo fieldInfo(field->getNameAsString(), 
                            field->getType().getAsString(),
                            fieldGuid);
        fieldInfo.attributes = fieldAttributes;
        
        structInfo.fields.push_back(fieldInfo);
    }
    
    database.AddStruct(structInfo);
}

std::vector<Attribute> ReflectionParser::ParseAttributes(const clang::Decl* decl)
{
    std::vector<Attribute> result;
    
    for (const auto* attr : decl->attrs()) {
        if (auto* annotateAttr = llvm::dyn_cast<clang::AnnotateAttr>(attr))
		{
            std::string annotation = annotateAttr->getAnnotation().str();
            
            if (annotation.find("GCLASS") == 0 || 
                annotation.find("GSTRUCT") == 0 || 
                annotation.find("GFIELD") == 0 || 
				annotation.find("GENUM_CLASS") == 0 || 
                annotation.find("GENUM_ITEM") == 0)
			{
                // Extract attributes from annotation
                std::regex attrRegex(R"(\b([A-Za-z0-9_]+)(?:\(([^)]*)\))?)");
                std::sregex_iterator it(annotation.begin(), annotation.end(), attrRegex);
                std::sregex_iterator end;
                
                for (; it != end; ++it)
				{
                    std::string attrName = (*it)[1].str();
                    std::string argsStr = (*it)[2].str();
                    
                    // Extract attribute arguments
                    std::vector<std::string> args;
                    if (!argsStr.empty())
					{
                        std::regex argRegex(R"(([^"]*))");
                        std::sregex_iterator argIt(argsStr.begin(), argsStr.end(), argRegex);
                        
                        for (; argIt != end; ++argIt)
						{
                            args.push_back((*argIt)[1].str());
                        }
                    }
                    
                    result.emplace_back(attrName, args);
                }
            }
        }
    }
    
    return result;
}

std::string ReflectionParser::ExtractGuid(const std::vector<Attribute>& attributes) {
    for (const auto& attr : attributes) {
        if (attr.name == "Guid" && !attr.arguments.empty()) {
            return attr.arguments[0];
        }
    }
    return "";
}

// AST Consumer for Clang
class ReflectionASTConsumer : public clang::ASTConsumer {
public:
    explicit ReflectionASTConsumer(ReflectionParser& parser) : parser(parser) {}
    
    void HandleTranslationUnit(clang::ASTContext& context) override {
        parser.ParseAST(context);
    }
    
private:
    ReflectionParser& parser;
};

// Frontend Action for Clang
class ReflectionFrontendAction : public clang::ASTFrontendAction {
public:
    ReflectionFrontendAction(ReflectionParser& parser, const std::string& outputDir)
        : parser(parser), outputDir(outputDir) {}
    
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef) override {
        return std::make_unique<ReflectionASTConsumer>(parser);
    }
    
    void EndSourceFileAction() override {
        parser.GenerateOutput(outputDir);
    }
    
private:
    ReflectionParser& parser;
    std::string outputDir;
};

} // namespace Gleam