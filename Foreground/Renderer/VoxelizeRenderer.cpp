#include "ForegroundCommon.h"
#include "GBufferRenderer.h"
#include "MegaPipeline.h"

namespace Foreground
{

CVoxelizeRenderer::CVoxelizeRenderer(CMegaPipeline* p)
    : Parent(p)
{
}

void CVoxelizeRenderer::SetRenderPass(RHI::CRenderPass::Ref renderPass, uint32_t subpass)
{
    RenderPass = std::move(renderPass);
    // Optionally rebuild all pipelines
}

void CVoxelizeRenderer::RenderList(RHI::IRenderContext& context,
                                   const std::vector<tc::Matrix3x4>& modelMats,
                                   const std::vector<CPrimitive*>& primitives)
{
    GarbageCollectResourceCache();
    if (modelMats.empty())
        return;

    BoundSet0 = false;
    for (size_t i = 0; i < modelMats.size(); i++)
        Render(context, modelMats[i], primitives[i]);
}

void CVoxelizeRenderer::PreparePrimitiveResources(std::shared_ptr<CPrimitive> primitive)
{
    auto& lib = PipelangContext.GetLibrary("Internal");

    // We have to do this since shader combination is statically done
    // Maybe a compile time table is a better solution
    if (auto triMesh = std::dynamic_pointer_cast<CTriangleMesh>(primitive->GetShape()))
    {
        if (auto basicMat = std::dynamic_pointer_cast<CBasicMaterial>(primitive->GetMaterial()))
        {
            RHI::CPipelineDesc desc;
            bool ok = lib.GetPipeline(desc,
                                      { "EngineCommon", "StandardTriMesh", "PerPrimitive",
                                        "StaticMeshPassThruVS", "GSTriInTriOut", "VoxelGS",
                                        "DefaultRasterizer", "BasicMaterialParams", "BasicMaterial",
                                        "VoxelData", "VoxelPS" });

            if (!ok)
                return;

            desc.PrimitiveTopology = triMesh->GetPrimitiveTopology();
            desc.RasterizerState.CullMode = RHI::ECullModeFlags::None;
            desc.DepthStencilState.DepthEnable = false;
            desc.RenderPass = RenderPass;
            desc.Subpass = 0;
            triMesh->PipelineSetVertexInputDesc(
                desc, lib.GetVertexAttribs("StandardTriMesh").GetAttributesByLocation());
            auto pipeline = RenderDevice->CreatePipeline(desc);

            CPrimitiveResources resources;
            resources.Pipeline = std::move(pipeline);
            resources.NodeDS = lib.GetParameterBlock("PerPrimitive").CreateDescriptorSet();
            CachedPrimitiveResources[primitive] = std::move(resources);
        }
    }
}

void CVoxelizeRenderer::ClearResourceCache() { CachedPrimitiveResources.clear(); }

void CVoxelizeRenderer::GarbageCollectResourceCache()
{
    for (auto iter = CachedPrimitiveResources.begin(); iter != CachedPrimitiveResources.end();
         iter++)
    {
        if (iter->first.expired())
            CachedPrimitiveResources.erase(iter);
    }
}

void CVoxelizeRenderer::Render(RHI::IRenderContext& context, const tc::Matrix3x4& modelMat,
                               CPrimitive* primitive)
{
    auto iter = CachedPrimitiveResources.find(primitive->weak_from_this());
    if (iter == CachedPrimitiveResources.end())
    {
        PreparePrimitiveResources(primitive->shared_from_this());
        iter = CachedPrimitiveResources.find(primitive->weak_from_this());
        if (iter == CachedPrimitiveResources.end())
            return;
    }

    auto& lib = PipelangContext.GetLibrary("Internal");
    if (!VoxelDS)
    {
        auto& pb = lib.GetParameterBlock("VoxelData");
        VoxelDS = pb.CreateDescriptorSet();
        pb.BindImageView(VoxelDS, Parent->getVoxelsImageView(), "voxels");
    }

    if (auto triMesh = std::dynamic_pointer_cast<CTriangleMesh>(primitive->GetShape()))
    {
        if (auto basicMat = std::dynamic_pointer_cast<CBasicMaterial>(primitive->GetMaterial()))
        {
            context.BindRenderPipeline(*iter->second.Pipeline);
            if (!BoundSet0)
            {
                Parent->BindEngineCommonForView(context, 2);
                BoundSet0 = true;
            }

            basicMat->Bind(context);

            PerPrimitiveConstants primitiveConstants;
            primitiveConstants.ModelMat = modelMat.ToMatrix4().Transpose();
            primitiveConstants.NormalMat = modelMat.ToMatrix3().Inverse();
            auto& pb = lib.GetParameterBlock("PerPrimitive");
            pb.BindConstants(iter->second.NodeDS, &primitiveConstants, sizeof(primitiveConstants),
                             "PerPrimitiveConstants");
            context.BindRenderDescriptorSet(2, *iter->second.NodeDS);

            context.BindRenderDescriptorSet(3, *VoxelDS);

            triMesh->Draw(context);
        }
    }
}

}
