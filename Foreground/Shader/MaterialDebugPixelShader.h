#pragma once
#include "ShaderCache.h"
#include <Pipeline.h>

namespace Foreground
{

template <typename TVertexShader, typename TMaterialShader> class CMaterialDebugPixelShader
{
public:
    void PreparePipelineDesc(RHI::CPipelineDesc& d)
    {
        if (!ShaderModule)
            Compile();
        d.PS = ShaderModule;
    }

protected:
    void Compile()
    {
        auto key = "GBufferPixelShader" + TVertexShader::GetInterpolantsHeader()
            + TMaterialShader::GetHeaderName();

        CShaderCompileEnvironment env;
        env.MainSourcePath = CResourceManager::Get().FindShader("GBufferPS.glsl");
        env.MainSourcePath = tc::FPathTools::FixSlashes(env.MainSourcePath);
        env.IncludeDirs.emplace_back(tc::FPathTools::StripFilename(env.MainSourcePath));
        env.StrReplaces["{{VSInterpolantsHeader}}"] = TVertexShader::GetInterpolantsHeader();
        env.StrReplaces["{{MaterialHeader}}"] = TMaterialShader::GetHeaderName();
        CompiledShaderModule = CShaderCache::Get().RetrieveOrCompileShader(key, std::move(env));
    }

private:
    RHI::CShaderModule::Ref ShaderModule;
};

}
