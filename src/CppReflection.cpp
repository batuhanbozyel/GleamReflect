#include "ReflectionParser.h"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace clang::tooling;
using namespace llvm;
using namespace Gleam;

// AST Consumer for Clang
class ReflectionASTConsumer : public clang::ASTConsumer
{
public:
    explicit ReflectionASTConsumer(ReflectionParser& parser)
        : mParser(parser)
    {
        
    }
    
    void HandleTranslationUnit(clang::ASTContext& context) override
    {
        mParser.ParseAST(context);
    }
    
private:
    ReflectionParser& mParser;
};

// Frontend Action for Clang
class ReflectionFrontendAction : public clang::ASTFrontendAction
{
public:
    ReflectionFrontendAction(ReflectionParser& parser, const std::string& outputDir)
        : mParser(parser), mOutputDir(outputDir)
    {
        
    }
    
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef) override
    {
        return std::make_unique<ReflectionASTConsumer>(mParser);
    }
    
    void EndSourceFileAction() override
    {
        mParser.GenerateOutput(mOutputDir);
    }
    
private:
    ReflectionParser& mParser;
    std::string mOutputDir;
};

static llvm::cl::OptionCategory CppReflectionCategory("C++ Reflection");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
    auto optionsParser = CommonOptionsParser::create(argc, argv, CppReflectionCategory);
    if (!optionsParser)
    {
        llvm::errs() << optionsParser.takeError();
        return 1;
    }
    CommonOptionsParser& parser = optionsParser.get();
    ClangTool tool(parser.getCompilations(),
                   parser.getSourcePathList());
    return tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
