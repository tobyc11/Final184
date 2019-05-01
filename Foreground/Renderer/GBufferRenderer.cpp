#include "GBufferRenderer.h"
#include "ForegroundCommon.h"
#include "MegaPipeline.h"
#include "Shader/BasicMaterialShader.h"
#include "Shader/GBufferPixelShader.h"
#include "Shader/ShaderFactory.h"
#include "Shader/StaticMeshVertexShader.h"

namespace Foreground
{

CGBufferRenderer::CGBufferRenderer(CMegaPipeline* p)
    : Parent(p)
{
}

void CGBufferRenderer::SetRenderPass(RHI::CRenderPass::Ref renderPass, uint32_t subpass)
{
    RenderPass = renderPass;
    // Optionally rebuild all pipelines
}

void CGBufferRenderer::RenderList(RHI::IRenderContext& context,
                                  const std::vector<tc::Matrix3x4>& modelMats,
                                  const std::vector<CPrimitive*>& primitives)
{
    GarbageCollectResourceCache();
    Parent->BindEngineCommon(context);
    for (size_t i = 0; i < modelMats.size(); i++)
        Render(context, modelMats[i], primitives[i]);
}

void CGBufferRenderer::PreparePrimitiveResources(std::shared_ptr<CPrimitive> primitive)
{
    // We have to do this since shader combination is statically done
    // Maybe a compile time table is a better solution
    if (auto triMesh = std::dynamic_pointer_cast<CTriangleMesh>(primitive->GetShape()))
    {
        if (auto basicMat = std::dynamic_pointer_cast<CBasicMaterial>(primitive->GetMaterial()))
        {
            auto vs = GlobalShaderFactory.GetShader<CStatcMeshVertexShader>();
            auto ps =
                GlobalShaderFactory
                    .GetShader<CGBufferPixelShader<CStatcMeshVertexShader, CBasicMaterialShader>>();
            RHI::CPipelineDesc desc;
            vs->PipelineSetVertexShader(desc);
            ps->PreparePipelineDesc(desc);
            desc.PrimitiveTopology = triMesh->GetPrimitiveTopology();
            desc.RasterizerState.CullMode = RHI::ECullModeFlags::None;
            desc.RenderPass = RenderPass;
            desc.Subpass = 0;
            triMesh->PipelineSetVertexInputDesc(desc, vs->GetAttributeLocationMap());
            auto pipeline = RenderDevice->CreatePipeline(desc);

            CPrimitiveResources resources;
            resources.Pipeline = std::move(pipeline);
            CachedPrimitiveResources[primitive] = std::move(resources);
        }
    }
}

void CGBufferRenderer::ClearResourceCache() { CachedPrimitiveResources.clear(); }

void CGBufferRenderer::GarbageCollectResourceCache()
{
    for (auto iter = CachedPrimitiveResources.begin(); iter != CachedPrimitiveResources.end();
         iter++)
    {
        if (iter->first.expired())
            CachedPrimitiveResources.erase(iter);
    }
}

void CGBufferRenderer::Render(RHI::IRenderContext& context, const tc::Matrix3x4& modelMat,
                              CPrimitive* primitive)
{
    auto iter = CachedPrimitiveResources.find(primitive->weak_from_this());
    if (iter == CachedPrimitiveResources.end())
    {
        PreparePrimitiveResources(primitive->shared_from_this());
        iter = CachedPrimitiveResources.find(primitive->weak_from_this());
    }

    if (auto triMesh = std::dynamic_pointer_cast<CTriangleMesh>(primitive->GetShape()))
    {
        if (auto basicMat = std::dynamic_pointer_cast<CBasicMaterial>(primitive->GetMaterial()))
        {
            CBasicMaterialShader::MaterialConstants materialConst;
            materialConst.BaseColor = basicMat->GetAlbedo();
            materialConst.MetallicRoughness.y = basicMat->GetMetallic();
            materialConst.MetallicRoughness.z = basicMat->GetRoughness();
            materialConst.UseTextures = 0;

            CStatcMeshVertexShader::PerPrimitiveConstants primitiveConstants;
            primitiveConstants.ModelMat = modelMat.ToMatrix4().Transpose();

            context.BindPipeline(*iter->second.Pipeline);
            CBasicMaterialShader::BindMaterialConstants(context, &materialConst);
            CStatcMeshVertexShader::BindPerPrimitiveConstants(context, &primitiveConstants);
            triMesh->Draw(context);
        }
    }
}

}
