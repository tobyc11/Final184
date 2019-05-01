#pragma once
#include "SceneGraph/Primitive.h"
#include <Matrix3x4.h>
#include <Pipeline.h>
#include <memory>
#include <unordered_map>

namespace Foreground
{

class CMegaPipeline;

class CGBufferRenderer
{
public:
    CGBufferRenderer(CMegaPipeline* p);

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

    struct CPrimitiveResources
    {
        RHI::CPipeline::Ref Pipeline;
    };

    RHI::CRenderPass::Ref RenderPass;
    std::unordered_map<std::weak_ptr<CPrimitive>, CPrimitiveResources> CachedPrimitiveResources;
};

}
