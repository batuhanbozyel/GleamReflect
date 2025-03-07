#include "ReflectionGenerator.h"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory ReflectionToolCategory("Reflection Generator Options");
static cl::opt<std::string> OutputDir("output-dir",
                                      cl::desc("Specify output directory for generated files"),
                                      cl::value_desc("directory"),
                                      cl::Required,
                                      cl::cat(ReflectionToolCategory));

struct DumpASTAction : public ASTFrontendAction
{
	std::unique_ptr<ASTConsumer>
	CreateASTConsumer(CompilerInstance &ci, StringRef inFile) override
	{
		return clang::CreateASTDumper(
			nullptr,/* dump to stdout */
			"", /* no filter */
			true, /* dump decls */
			true, /* deserialize */
			false /* don't dump lookups */
		);
	}
};

int main(int argc, const char **argv)
{
    CommonOptionsParser optionsParser(argc, argv, ReflectionToolCategory);
    ClangTool Tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());
    
    // Set up the reflection parser
    Gleam::ReflectionParser parser;
    Gleam::ReflectionFrontendAction frontendAction(parser, OutputDir);
    
    // Run the Clang tool
    return Tool.run(newFrontendActionFactory(&frontendAction).get());
}