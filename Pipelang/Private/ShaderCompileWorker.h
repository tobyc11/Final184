#pragma once
#include <Device.h>
#include <ShaderModule.h>
#include <future>
#include <map>
#include <string>

namespace Pl
{

struct CShaderCompileEnvironment
{
    std::string MainSourcePath;
    std::string ShaderStage;
    std::vector<std::string> IncludeDirs;
    std::map<std::string, std::string> Definitions;
    std::map<std::string, std::string> StrReplaces;
};

class CShaderCompileWorker
{
public:
    explicit CShaderCompileWorker(CShaderCompileEnvironment env);

    void SetOutputPath(std::string path) { OutputPath = std::move(path); }
    RHI::CShaderModule::Ref Compile(const RHI::CDevice::Ref& device);
    std::future<RHI::CShaderModule::Ref> CompileAsync();

private:
    std::string OutputPath;
    CShaderCompileEnvironment CompileEnv;
};

}
