#pragma once
#include "VertexShaderCommon.h"
#include <Pipeline.h>

namespace Foreground
{

// Should only be instanciated once per renderer
class CStatcMeshVertexShader
{
public:
    uint32_t GetAttributeLocation(EAttributeType type) const;
    static std::string GetInterpolantsHeader();

    void Compile();
    void PipelineSetVertexShader(RHI::CPipelineDesc& desc);

private:
    RHI::CShaderModule::Ref CompiledShaderModule;
};

}
