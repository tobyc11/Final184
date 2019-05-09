#pragma once
#include "SceneGraph/Primitive.h"
#include <Matrix3x4.h>
#include <Pipeline.h>
#include <map>
#include <memory>
#include <unordered_map>

namespace Foreground
{

class CMegaPipeline;

class CVoxelizeRenderer
{
public:
    explicit CVoxelizeRenderer(CMegaPipeline* p);

    void SetRenderPass(RHI::CRenderPass::Ref renderPass, uint32_t subpass = 0);

    void RenderList(RHI::IRenderContext& context, const std::vector<tc::Matrix3x4>& modelMats,
                    const std::vector<CPrimitive*>& primitives);

    void PreparePrimitiveResources(std::shared_ptr<CPrimitive> primitive);
    void ClearResourceCache();
    void GarbageCollectResourceCache();

protected:
    void Render(RHI::IRenderContext& context, const tc::Matrix3x4& modelMat, CPrimitive* primitive);

private:
    CMegaPipeline* Parent;

    struct PerPrimitiveConstants
    {
        tc::Matrix4 ModelMat;
        tc::Matrix4 NormalMat;
    };

    struct CPrimitiveResources
    {
        RHI::CPipeline::Ref Pipeline;
        RHI::CDescriptorSet::Ref NodeDS;
    };

    RHI::CRenderPass::Ref RenderPass;
    std::map<std::weak_ptr<CPrimitive>, CPrimitiveResources,
             std::owner_less<std::weak_ptr<CPrimitive>>>
        CachedPrimitiveResources;

    // Temporary render flags
    bool BoundSet0 = false;
};

}
