#pragma once
#include <ShaderModule.h>
#include <future>
#include <map>
#include <string>

namespace Foreground
{

struct CShaderCompileEnvironment
{
    std::string MainSourcePath;
    std::vector<std::string> IncludeDirs;
    std::map<std::string, std::string> Definitions;
    std::map<std::string, std::string> StrReplaces;
};

class CShaderCompileWorker
{
public:
    CShaderCompileWorker(CShaderCompileEnvironment env);

    void SetOutputPath(std::string path) { OutputPath = std::move(path); }
    RHI::CShaderModule::Ref Compile();
    std::future<RHI::CShaderModule::Ref> CompileAsync();

private:
    std::string OutputPath;
    CShaderCompileEnvironment CompileEnv;
};

}
