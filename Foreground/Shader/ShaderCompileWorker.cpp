#include "ShaderCompileWorker.h"
#include "ForegroundCommon.h"
#include <fstream>
#include <sstream>

namespace Foreground
{

CShaderCompileWorker::CShaderCompileWorker(CShaderCompileEnvironment env)
    : CompileEnv(std::move(env))
{
}

RHI::CShaderModule::Ref CShaderCompileWorker::Compile()
{
    // TODO:
    // It's better to create a RPC wrapper for glslc
    // For now just invoke the executable and see
    std::stringstream ss;
    ss << "glslc";
    for (const auto& incDir : CompileEnv.IncludeDirs)
        ss << " -I" << incDir;
    for (const auto& pair : CompileEnv.Definitions)
        ss << " -D" << pair.first << "=" << pair.second;
    ss << " " << CompileEnv.MainSourcePath;
    ss << " -o " << OutputPath;
    std::system(ss.str().c_str());

    std::ifstream file(OutputPath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return nullptr;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return RenderDevice->CreateShaderModule(buffer.size(), buffer.data());
    return nullptr;
}

std::future<RHI::CShaderModule::Ref> CShaderCompileWorker::CompileAsync() { throw "unimplemented"; }

}
