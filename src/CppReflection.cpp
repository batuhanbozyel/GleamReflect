#include "ReflectionParser.h"
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace Gleam;

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

class ReflectionFrontendAction : public clang::ASTFrontendAction
{
public:
    ReflectionFrontendAction(const std::string& outputDir)
        : mParser()
        , mOutputDir(outputDir)
    {
        
    }
    
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef file) override
    {
        return std::make_unique<ReflectionASTConsumer>(mParser);
    }
    
    void EndSourceFileAction() override
    {
        mParser.GenerateOutput(mOutputDir);
    }
    
private:
    ReflectionParser mParser;
    std::string mOutputDir;
};

class ReflectionActionFactory : public clang::tooling::FrontendActionFactory
{
 public:
    explicit ReflectionActionFactory(const std::string& outputPath)
        : mOutputPath(outputPath)
    {
        
    }

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<ReflectionFrontendAction>(mOutputPath);
    }

 private:
    std::string mOutputPath;
};

static llvm::cl::OptionCategory CppReflectionCategory("C++ Reflection");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
    auto optionsParser = clang::tooling::CommonOptionsParser::create(argc, argv, CppReflectionCategory);
    if (!optionsParser)
    {
        llvm::errs() << optionsParser.takeError();
        return 1;
    }
    clang::tooling::CommonOptionsParser& parser = optionsParser.get();
    clang::tooling::ClangTool tool(parser.getCompilations(),
                                   parser.getSourcePathList());
    
    tool.appendArgumentsAdjuster([](const clang::tooling::CommandLineArguments& args, llvm::StringRef filename) -> clang::tooling::CommandLineArguments
    {
        clang::tooling::CommandLineArguments adjustedArgs = args;
        adjustedArgs.push_back("-D__GLEAM_REFLECTION__");
        return adjustedArgs;
    });
    return tool.run(clang::tooling::newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
