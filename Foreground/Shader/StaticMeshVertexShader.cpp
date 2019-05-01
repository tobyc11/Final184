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
    case EAttributeType::Position:
        return 0;
    case EAttributeType::Normal:
        return 1;
    case EAttributeType::Tangent:
        return 2;
    case EAttributeType::TexCoord0:
        return 3;
    case EAttributeType::TexCoord1:
    case EAttributeType::Color0:
    case EAttributeType::Joints0:
    case EAttributeType::Weights0:
    default:
        throw std::runtime_error("Wrong shader???");
    }
}

std::map<EAttributeType, uint32_t> CStatcMeshVertexShader::GetAttributeLocationMap() const
{
    using E = EAttributeType;
    return { { E::Position, 0 }, { E::Normal, 1 }, { E::Tangent, 2 }, { E::TexCoord0, 3 } };
}

std::string CStatcMeshVertexShader::GetInterpolantsHeader()
{
    return std::string("StaticMeshInterpolants.h");
}

void CStatcMeshVertexShader::Compile()
{
#define CACHE_KEY "StatcMeshVertexShader"
    CShaderCompileEnvironment env;
    env.ShaderStage = "vertex";
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
