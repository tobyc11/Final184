#pragma once
#include "ShaderCompileWorker.h"
#include <LangUtils.h>
#include <ShaderModule.h>
#include <mutex>
#include <unordered_map>

namespace Foreground
{

class CShaderCache : public tc::FNonCopyable
{
public:
    static CShaderCache& Get();

    void InsertShader(const std::string& key, RHI::CShaderModule::Ref shaderModule);
    RHI::CShaderModule::Ref RetrieveShader(const std::string& key);
    RHI::CShaderModule::Ref RetrieveOrCompileShader(const std::string& key,
                                                    CShaderCompileEnvironment env);

private:
    std::mutex ShaderCacheMutex;
    std::unordered_map<std::string, RHI::CShaderModule::Ref> ShaderHashMap;
    std::unordered_map<std::string, uint64_t> LastCompileTimestamp;
};

}
