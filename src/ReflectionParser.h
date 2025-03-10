#pragma once
#include "Reflection/Attribute.h"
#include "Reflection/Meta.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>
#include <filesystem>

namespace clang {
    class ASTContext;
    class Decl;
    class EnumDecl;
    class CXXRecordDecl;
    class FieldDecl;
}

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
    void GenerateOutput(const std::string& outputDir);
    
private:
    void HandleEnumDecl(const clang::EnumDecl* enumDecl);
    void HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
    
    std::vector<AttributePair> ParseAttributes(const clang::Decl* decl);
    Reflection::Attribute::Guid ExtractGuid(const std::vector<AttributePair>& attributes);
};

} // namespace Gleam
