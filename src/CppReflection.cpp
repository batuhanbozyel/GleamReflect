#include "ReflectionParser.h"

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/AllTUsExecution.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <llvm/Support/CommandLine.h>

#include <filesystem>
#include <algorithm>
#include <thread>
#include <memory>

using namespace Gleam;

class ReflectionCompilationDatabase : public clang::tooling::CompilationDatabase
{
public:
	ReflectionCompilationDatabase(const clang::tooling::CompilationDatabase& base,
								   const std::vector<std::string>& sourceFiles)
		: mBase(base), mSourceFiles(sourceFiles)
	{
	}
	
	std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override
	{
		return mBase.getCompileCommands(FilePath);
	}
	
	std::vector<std::string> getAllFiles() const override
	{
		return mSourceFiles;
	}
	
	std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override
	{
		return mBase.getAllCompileCommands();
	}
	
private:
	const clang::tooling::CompilationDatabase& mBase;
	std::vector<std::string> mSourceFiles;
};

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

static llvm::cl::opt<bool> LogTrace("log-trace",
									llvm::cl::desc("Enable logging trace"),
									llvm::cl::value_desc("log"),
									llvm::cl::Optional,
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
	const auto& sourceFiles = parser.getSourcePathList();
	if (sourceFiles.empty())
	{
		llvm::errs() << "No source files provided\n";
		return 1;
	}
	
	if (LogTrace)
	{
		llvm::outs() << "Processing " << sourceFiles.size() << " header files:\n";
		for (const auto& file : sourceFiles)
		{
			llvm::outs() << "  " << file << "\n";
		}
		llvm::outs() << "\n";
		llvm::outs().flush();
	}
	
	auto reflectionCompilationDB = ReflectionCompilationDatabase(parser.getCompilations(), sourceFiles);
	auto executor = clang::tooling::AllTUsToolExecutor(reflectionCompilationDB, 0);
	auto adjuster = clang::tooling::getInsertArgumentAdjuster(
			{"-D__GLEAM_REFLECTION__", "--no-warnings"},
			clang::tooling::ArgumentInsertPosition::END);
	
	if (LogTrace)
	{
		llvm::outs() << "Starting reflection generation...\n";
		llvm::outs().flush();
	}
	
	ReflectionParser reflectionParser;
	std::pair<std::unique_ptr<clang::tooling::FrontendActionFactory>, clang::tooling::ArgumentsAdjuster> frontend = std::pair{std::make_unique<ReflectionFrontendActionFactory>(reflectionParser), adjuster};
	llvm::Error err = executor.execute(llvm::ArrayRef<decltype(frontend)>(frontend));
	if (err)
	{
		llvm::errs() << "Execution failed: " << llvm::toString(std::move(err)) << "\n";
		return 1;
	}
	
	std::string moduleName = Module;
	std::string headerDirectory = HeaderDir;
	std::string binaryDirectory = BinaryDir;
	reflectionParser.GenerateOutput(moduleName, headerDirectory, binaryDirectory);

	if (LogTrace)
	{
		llvm::outs() << "Reflection generation completed successfully\n";
		llvm::outs().flush();
	}
	
	return 0;
}
