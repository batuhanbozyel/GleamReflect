#include "ReflectionParser.h"

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
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
    
    explicit ReflectionFrontendAction(ReflectionParser& parser)
		: mParser(parser)
    {
        
    }
    
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef file) override
    {
        return std::make_unique<ReflectionASTConsumer>(mParser);
    }
    
private:
    
    ReflectionParser& mParser;
};

class ReflectionFrontendActionFactory : public clang::tooling::FrontendActionFactory
{
public:
    
    explicit ReflectionFrontendActionFactory(ReflectionParser& parser)
        : mParser(parser)
	{
		
	}
    
    virtual std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<ReflectionFrontendAction>(mParser);
    }
    
private:
    
	ReflectionParser& mParser;
};

static llvm::cl::OptionCategory ReflectionToolCategory("C++ Reflection");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

static llvm::cl::opt<std::string> Module("module",
                                         llvm::cl::desc("Specify module name for reflection"),
                                         llvm::cl::value_desc("name"),
                                         llvm::cl::Required,
                                         llvm::cl::cat(ReflectionToolCategory));

static llvm::cl::opt<std::string> HeaderDir("header-dir",
                                            llvm::cl::desc("Specify output directory for generated header"),
                                            llvm::cl::value_desc("directory"),
                                            llvm::cl::Required,
                                            llvm::cl::cat(ReflectionToolCategory));

static llvm::cl::opt<std::string> BinaryDir("binary-dir",
                                            llvm::cl::desc("Specify output directory for database"),
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
		adjustedArgs.push_back("--no-warnings");
        return adjustedArgs;
    });
    
	ReflectionParser reflectionParser;
    auto frontendActionFactory = std::make_unique<ReflectionFrontendActionFactory>(reflectionParser);

    int result = tool.run(frontendActionFactory.get());
	if (result == 0)
	{
		std::string moduleName = Module;
		std::string headerDirectory = HeaderDir;
		std::string binaryDirectory = BinaryDir;
		reflectionParser.GenerateOutput(moduleName, headerDirectory, binaryDirectory);
	}
	return result;
}
