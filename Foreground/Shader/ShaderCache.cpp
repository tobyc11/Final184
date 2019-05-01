#include "ShaderCache.h"

namespace Foreground
{

CShaderCache& CShaderCache::Get()
{
    static CShaderCache singleton;
    return singleton;
}

void CShaderCache::InsertShader(const std::string& key, RHI::CShaderModule::Ref shaderModule)
{
    std::lock_guard<std::mutex> lk(ShaderCacheMutex);

    assert(shaderModule);
    ShaderHashMap[key] = shaderModule;
}

RHI::CShaderModule::Ref CShaderCache::RetrieveShader(const std::string& key)
{
    auto iter = ShaderHashMap.find(key);
    if (iter == ShaderHashMap.end())
        return nullptr;
    return iter->second;
}

RHI::CShaderModule::Ref CShaderCache::RetrieveOrCompileShader(const std::string& key,
                                                              CShaderCompileEnvironment env)
{
    auto iter = ShaderHashMap.find(key);
    if (iter != ShaderHashMap.end())
        return iter->second;

    std::lock_guard<std::mutex> lk(ShaderCacheMutex);
    CShaderCompileWorker worker(std::move(env));
    auto shader = worker.Compile();
    ShaderHashMap[key] = shader;
    return shader;
}
}
