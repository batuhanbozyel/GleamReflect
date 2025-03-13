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
    using EnumMap = std::unordered_map<Reflection::Attribute::Guid, Reflection::EnumDescription>;
    using ClassMap = std::unordered_map<Reflection::Attribute::Guid, Reflection::ClassDescription>;
public:
    ReflectionParser();
    void ParseAST(clang::ASTContext& context);
    void GenerateOutput(const std::string& outputDir);
    
private:
    EnumMap::iterator HandleEnumDecl(const clang::EnumDecl* enumDecl);
    ClassMap::iterator HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
    
    std::vector<AttributePair> ParseAttributes(const std::string& annotation);
    Reflection::Attribute::Guid ExtractGuid(const std::vector<AttributePair>& attributes);
    
    // TODO: move these to Database
    EnumMap mGuidToEnum;
    ClassMap mGuidToClass;
};

} // namespace Gleam
