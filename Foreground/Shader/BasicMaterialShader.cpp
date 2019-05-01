#include "BasicMaterialShader.h"

namespace Foreground
{

void CBasicMaterialShader::BindMaterialConstants(RHI::IRenderContext& context,
                                                 MaterialConstants* data)
{
    context.BindConstants(data, sizeof(MaterialConstants), 1, 0, 0);
}

void CBasicMaterialShader::BindBaseColorTex(RHI::IRenderContext& context, RHI::CImageView& img)
{
    context.BindImageView(img, 1, 1, 0);
}

void CBasicMaterialShader::BindMetallicRoughnessTex(RHI::IRenderContext& context,
                                                    RHI::CImageView& img)
{
    context.BindImageView(img, 1, 2, 0);
}

}
