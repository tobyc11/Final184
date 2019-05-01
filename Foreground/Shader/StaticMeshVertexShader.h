#pragma once
#include "VertexShaderCommon.h"
#include <Matrix4.h>
#include <Pipeline.h>
#include <RenderContext.h>
#include <map>

namespace Foreground
{

// Should only be instanciated once per renderer
class CStatcMeshVertexShader
{
public:
    struct PerPrimitiveConstants
    {
        tc::Matrix4 ModelMat;
    };
    static void BindPerPrimitiveConstants(RHI::IRenderContext& context,
                                          PerPrimitiveConstants* data);

    uint32_t GetAttributeLocation(EAttributeType type) const;
    std::map<EAttributeType, uint32_t> GetAttributeLocationMap() const;
    static std::string GetInterpolantsHeader();

    void Compile();
    void PipelineSetVertexShader(RHI::CPipelineDesc& desc);

private:
    RHI::CShaderModule::Ref CompiledShaderModule;
};

}
