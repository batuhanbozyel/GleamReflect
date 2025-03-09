#pragma once
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

class ReflectionParser
{
public:
    ReflectionParser();
    bool ParseAST(clang::ASTContext& context);
    void GenerateOutput(const std::string& outputDir);
    
private:
    void HandleEnumDecl(const clang::EnumDecl* enumDecl);
    void HandleRecordDecl(const clang::CXXRecordDecl* recordDecl);
};

} // namespace Gleam
