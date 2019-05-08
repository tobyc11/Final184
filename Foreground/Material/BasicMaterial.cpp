#include "BasicMaterial.h"
#include "ForegroundCommon.h"

namespace Foreground
{

void CBasicMaterial::Bind(RHI::IRenderContext& context)
{
    if (bDSDirty)
    {
        auto& lib = PipelangContext.GetLibrary("Internal");
        auto& pb = lib.GetParameterBlock("BasicMaterialParams");
        if (!DescriptorSet)
            DescriptorSet = pb.CreateDescriptorSet();

        MaterialConstants materialConst;
        materialConst.BaseColor = GetAlbedo();
        materialConst.MetallicRoughness.y = GetMetallic();
        materialConst.MetallicRoughness.z = GetRoughness();
        materialConst.UseTextures =
            GetAlbedoImage() || GetMetallicRoughnessImage() ? 1 : 0;

        pb.BindConstants(DescriptorSet, &materialConst, sizeof(materialConst), "MaterialConstants");
        pb.BindImageView(DescriptorSet, GetAlbedoImage(), "BaseColorTex");
        pb.BindImageView(DescriptorSet, GetMetallicRoughnessImage(), "MetallicRoughnessTex");

        bDSDirty = false;
    }
    context.BindRenderDescriptorSet(1, *DescriptorSet);
}

}
