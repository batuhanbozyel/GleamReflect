#pragma once
#include "Reflection/Database.h"

#include <string>
#include <vector>
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
    void GenerateOutput(const std::filesystem::path& outputDir);
    
private:
    Reflection::Database::EnumMap::iterator HandleEnumDecl(const clang::EnumDecl* enumDecl);
    Reflection::Database::ClassMap::iterator HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
    
    std::vector<AttributePair> ParseAttributes(const std::string& annotation);
    Reflection::Attribute::Guid ExtractGuid(const std::vector<AttributePair>& attributes);
private:
    Reflection::Database mDatabase;
};

} // namespace Gleam
