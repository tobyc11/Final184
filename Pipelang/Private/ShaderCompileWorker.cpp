#include "ShaderCompileWorker.h"
#include <fstream>
#include <sstream>

namespace Pl
{

CShaderCompileWorker::CShaderCompileWorker(CShaderCompileEnvironment env)
    : CompileEnv(std::move(env))
{
}

RHI::CShaderModule::Ref CShaderCompileWorker::Compile()
{
    assert(!CompileEnv.ShaderStage.empty());

    if (!CompileEnv.StrReplaces.empty())
    {
        std::ifstream sourceFile(CompileEnv.MainSourcePath);
        std::string sourceStr((std::istreambuf_iterator<char>(sourceFile)),
                              std::istreambuf_iterator<char>());

        for (const auto& pair : CompileEnv.StrReplaces)
        {
            std::string::size_type pos = 0u;
            while ((pos = sourceStr.find(pair.first, pos)) != std::string::npos)
            {
                sourceStr.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length();
            }
        }

        std::ofstream tempSrc("temp.glsl", std::ios ::out);
        tempSrc << sourceStr;
        tempSrc.close();
        CompileEnv.MainSourcePath = "temp.glsl";
    }

    // TODO:
    // It's better to create a RPC wrapper for glslc
    // For now just invoke the executable and see
    std::stringstream ss;
    ss << "glslc";
#ifdef DEBUG
    ss << " -g -O0";
#else
    ss << " -O";
#endif
    ss << " -fshader-stage=" << CompileEnv.ShaderStage;
    for (const auto& incDir : CompileEnv.IncludeDirs)
        ss << " -I" << incDir;
    for (const auto& pair : CompileEnv.Definitions)
        ss << " -D" << pair.first << "=" << pair.second;
    ss << " " << CompileEnv.MainSourcePath;
    ss << " -o " << OutputPath;
    printf("Running: %s\n", ss.str().c_str());
    std::system(ss.str().c_str());

    std::ifstream file(OutputPath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return nullptr;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return nullptr; //return RenderDevice->CreateShaderModule(buffer.size(), buffer.data());
    return nullptr;
}

std::future<RHI::CShaderModule::Ref> CShaderCompileWorker::CompileAsync() { throw "unimplemented"; }

}
