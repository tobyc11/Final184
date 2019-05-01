#include "StaticMeshVertexShader.h"
#include "ForegroundCommon.h"
#include "Resources/ResourceManager.h"
#include "ShaderCache.h"
#include <PathTools.h>

namespace Foreground
{

uint32_t CStatcMeshVertexShader::GetAttributeLocation(EAttributeType type) const
{
    switch (type)
    {
    case Foreground::EAttributeType::Position:
        return 0;
    case Foreground::EAttributeType::Normal:
        return 1;
    case Foreground::EAttributeType::Tangent:
        return 2;
    case Foreground::EAttributeType::TexCoord0:
        return 3;
    case Foreground::EAttributeType::TexCoord1:
    case Foreground::EAttributeType::Color0:
    case Foreground::EAttributeType::Joints0:
    case Foreground::EAttributeType::Weights0:
    default:
        throw std::runtime_error("Wrong shader???");
    }
}

std::string CStatcMeshVertexShader::GetInterpolantsHeader()
{
    return std::string("StaticMeshInterpolants.h");
}

void CStatcMeshVertexShader::Compile()
{
#define CACHE_KEY "StatcMeshVertexShader"
    CShaderCompileEnvironment env;
    env.MainSourcePath = CResourceManager::Get().FindShader("StaticMeshVS.glsl");
    env.MainSourcePath = tc::FPathTools::FixSlashes(env.MainSourcePath);
    env.IncludeDirs.emplace_back(tc::FPathTools::StripFilename(env.MainSourcePath));
    CompiledShaderModule = CShaderCache::Get().RetrieveOrCompileShader(CACHE_KEY, std::move(env));
}

void CStatcMeshVertexShader::PipelineSetVertexShader(RHI::CPipelineDesc& desc)
{
    if (!CompiledShaderModule)
        Compile();
    desc.VS = CompiledShaderModule;
}

}
