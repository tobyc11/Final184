#pragma once
#include <Pipeline.h>
#include <RenderContext.h>
#include <glm/glm.hpp>

namespace Foreground
{

// Header only shader, cannot be compiled individually
class CBasicMaterialShader
{
public:
    // Shader interface: this should mirror the shader code
    struct MaterialConstants
    {
        glm::vec4 BaseColor;
        glm::vec4 MetallicRoughness;
        bool UseTextures;
    };

    void BindMaterialConstants(RHI::IRenderContext& context, MaterialConstants* data);
    void BindBaseColorTex(RHI::IRenderContext& context, RHI::CImageView& img);
    void BindMetallicRoughnessTex(RHI::IRenderContext& context, RHI::CImageView& img);

    static std::string GetHeaderName() { return "BasicMaterial.h"; }
};

}
