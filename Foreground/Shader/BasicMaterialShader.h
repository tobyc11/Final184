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
        tc::Vector4 BaseColor;
        tc::Vector4 MetallicRoughness;
        bool UseTextures;
    };

    static void BindMaterialConstants(RHI::IRenderContext& context, MaterialConstants* data);
    static void BindBaseColorTex(RHI::IRenderContext& context, RHI::CImageView& img);
    static void BindMetallicRoughnessTex(RHI::IRenderContext& context, RHI::CImageView& img);

    static std::string GetHeaderName() { return "BasicMaterial.h"; }
};

}
