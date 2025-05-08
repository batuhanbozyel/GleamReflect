#include "ReflectionParser.h"

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include <filesystem>

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
    
    explicit ReflectionFrontendAction(const std::filesystem::path& outputDir)
        : mOutputDir(outputDir)
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
    std::filesystem::path mOutputDir;
};

class ReflectionFrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
    
    explicit ReflectionFrontendActionFactory(const std::filesystem::path& outputDir)
        : mOutputDir(outputDir)
    {
        
    }
    
    virtual std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<ReflectionFrontendAction>(mOutputDir);
    }
    
private:
    
    std::filesystem::path mOutputDir;
};

static llvm::cl::OptionCategory ReflectionToolCategory("C++ Reflection");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

static llvm::cl::opt<std::string> OutputDir("output-dir",
                                            llvm::cl::desc("Specify output directory for generated files"),
                                            llvm::cl::value_desc("directory"),
                                            llvm::cl::Required,
                                            llvm::cl::cat(ReflectionToolCategory));

int main(int argc, const char **argv)
{
    auto optionsParser = clang::tooling::CommonOptionsParser::create(argc, argv, ReflectionToolCategory);
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
    
	std::string outputDirectory = OutputDir;
    auto frontendActionFactory = std::make_unique<ReflectionFrontendActionFactory>(outputDirectory);
    return tool.run(frontendActionFactory.get());
}
