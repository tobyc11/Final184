#include "BasicMaterial.h"
#include "ForegroundCommon.h"

#include <imgui.h>

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
        materialConst.MetallicRoughness.z = GetMetallic();
        materialConst.MetallicRoughness.y = GetRoughness();
        materialConst.UseTextures = GetAlbedoImage() || GetMetallicRoughnessImage() ? 1 : 0;

        MaterialConstantsBuffer = RenderDevice->CreateBuffer(
            sizeof(MaterialConstants), RHI::EBufferUsageFlags::ConstantBuffer, &materialConst);

        pb.BindBuffer(DescriptorSet, MaterialConstantsBuffer, 0, sizeof(MaterialConstants),
                      "MaterialConstants");
        pb.BindImageView(DescriptorSet, GetAlbedoImage(), "BaseColorTex");
        if (GetAlbedoImage() && !GetMetallicRoughnessImage())
            // TODO: replace this with a dummy texture
            pb.BindImageView(DescriptorSet, GetAlbedoImage(), "MetallicRoughnessTex");
        else
            pb.BindImageView(DescriptorSet, GetMetallicRoughnessImage(), "MetallicRoughnessTex");

        bDSDirty = false;
    }
    context.BindRenderDescriptorSet(1, *DescriptorSet);
}

void CBasicMaterial::ImGuiEditor()
{
    if (ImGui::CollapsingHeader("Basic Material"))
    {
        if (ImGui::ColorEdit3("Albedo", &Albedo.x))
            bDSDirty = true;
        if (ImGui::DragFloat("Metallic", &Metallic, 0.01f, 0.0f, 1.0f))
            bDSDirty = true;
        if (ImGui::DragFloat("Roughness", &Roughness, 0.01f, 0.0f, 1.0f))
            bDSDirty = true;
    }
}

}
