#include "ReflectionParser.h"

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include <filesystem>
#include <algorithm>
#include <thread>

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

	std::vector<std::string> headerFiles;
	headerFiles.reserve(sourceFiles.size());
	std::copy_if(sourceFiles.begin(), sourceFiles.end(), std::back_inserter(headerFiles), [](const std::string& file) -> bool
	{
		auto extensionSeperator = file.find_last_of('.');
		if (extensionSeperator != std::string::npos)
		{
			auto extension = file.substr(extensionSeperator);
			return extension == ".h" || extension == ".hpp" || extension == ".hxx" ||
				extension == ".h++" || extension == ".hh" || extension == ".inc";
		}
		return false;
	});

	uint32_t numThreads = std::min(std::thread::hardware_concurrency(), static_cast<uint32_t>(sourceFiles.size()));
	std::vector<int> threadResults(numThreads, 0);
	std::vector<std::thread> parserThreads;
	parserThreads.reserve(numThreads);

	std::mutex logMutex;
	bool logTrace = LogTrace;
	ReflectionParser reflectionParser;
	for (uint32_t threadId = 0; threadId < numThreads; ++threadId)
	{
		parserThreads.emplace_back([&logMutex, &reflectionParser, &threadResults, &parser, &headerFiles, logTrace, numThreads, threadId]()
		{
			uint32_t numFilesPerThread = static_cast<uint32_t>(std::ceil(static_cast<float>(headerFiles.size()) / static_cast<float>(numThreads)));
			uint32_t numFiles = std::min(numFilesPerThread, (uint32_t)headerFiles.size() - numFilesPerThread * threadId);

			std::vector<std::string> files;
			files.reserve(numFiles);

			for (uint32_t i = 0; i < numFiles; ++i)
			{
				files.emplace_back(headerFiles[numFilesPerThread * threadId + i]);
			}

			if (logTrace)
			{
				std::lock_guard guard(logMutex);
				llvm::outs() << "Thread " << threadId << " processing files: ";
				for (const auto& file : files)
				{
					llvm::outs() << file << " ";
				}
				llvm::outs() << "\n";
			}

			llvm::ArrayRef<std::string> filesRef(files.data(), files.size());
			clang::tooling::ClangTool tool(parser.getCompilations(), filesRef);
			tool.appendArgumentsAdjuster([](const clang::tooling::CommandLineArguments& args, llvm::StringRef filename) -> clang::tooling::CommandLineArguments
			{
				clang::tooling::CommandLineArguments adjustedArgs = args;
				adjustedArgs.push_back("-D__GLEAM_REFLECTION__");
				adjustedArgs.push_back("--no-warnings");
				return adjustedArgs;
			});

			auto frontendActionFactory = std::make_unique<ReflectionFrontendActionFactory>(reflectionParser);
			threadResults[threadId] = tool.run(frontendActionFactory.get());
		});
	}

	for (auto& thread : parserThreads)
	{
		thread.join();
	}

	for (uint32_t i = 0; i < numThreads; ++i)
	{
		if (threadResults[i] != 0)
		{
			llvm::errs() << "Thread " << i << " failed with code: " << threadResults[i] << "\n";
			return threadResults[i];
		}
	}

	std::string moduleName = Module;
	std::string headerDirectory = HeaderDir;
	std::string binaryDirectory = BinaryDir;
	reflectionParser.GenerateOutput(moduleName, headerDirectory, binaryDirectory);

	return 0;
}
