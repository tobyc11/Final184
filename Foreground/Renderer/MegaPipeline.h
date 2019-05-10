#pragma once
#include "ForegroundCommon.h"
#include "GBufferRenderer.h"
#include "SceneGraph/SceneView.h"
#include "VoxelizeRenderer.h"
#include "ZOnlyRenderer.h"
#include <Pipeline.h>
#include <Resources.h>
#include <Sampler.h>

#include <Components/Material.h>

namespace Foreground
{

struct alignas(16) PreviousProjections
{
    tc::Matrix4 PrevProjection;
    tc::Matrix4 PrevModelView;
};

class CMegaPipeline
{
public:
    explicit CMegaPipeline(RHI::CSwapChain::Ref swapChain);

    void SetSceneView(std::unique_ptr<CSceneView> sceneView,
                      std::unique_ptr<CSceneView> shadowView,
                      std::unique_ptr<CSceneView> voxelizerSceneView);

    void Resize();
    void Render();

    RHI::CImageView::Ref getVoxelsImageView() const { return VoxelBuffer; };

    void BindEngineCommonForView(RHI::IRenderContext& context, uint32_t viewIndex);

protected:
    void CreateRenderPasses();
    void CreateGBufferPass(uint32_t width, uint32_t height);
    void CreateShadowBufferPass();
    void CreateScreenPass();
    void CreateVoxelizePass();

private:
    RHI::CSwapChain::Ref SwapChain;
    std::unique_ptr<CSceneView> SceneView;
    std::unique_ptr<CSceneView> ShadowSceneView;
    std::unique_ptr<CSceneView> VoxelizerSceneView;

    PreviousProjections prevProj;

    uint32_t frameCount = 0;

    uint32_t width;
    uint32_t height;

    RHI::CImage::Ref VoxelImage;
    RHI::CImage::Ref taaImageA;
    RHI::CImage::Ref taaImageB;

    RHI::CImageView::Ref GBuffer0;
    RHI::CImageView::Ref GBuffer1;
    RHI::CImageView::Ref GBuffer2;
    // RHI::CImageView::Ref GBuffer3;
    RHI::CImageView::Ref GBufferDepth;
    RHI::CImageView::Ref ShadowDepth;
    RHI::CImageView::Ref VoxelBuffer;
    RHI::CImageView::Ref taaImageView;
    RHI::CRenderPass::Ref GBufferPass;
    RHI::CRenderPass::Ref VoxelizationPass;
    RHI::CRenderPass::Ref ZOnlyPass;

    RHI::CSampler::Ref GlobalNiceSampler;
    RHI::CSampler::Ref GlobalLinearSampler;
    RHI::CSampler::Ref GlobalNearestSampler;
    RHI::CPipeline::Ref BlitPipeline;

    CGBufferRenderer GBufferRenderer;
    CZOnlyRenderer ZOnlyRenderer;
    CVoxelizeRenderer VoxelizeRenderer;

    std::shared_ptr<CMaterial> gtao_visibility;
    std::shared_ptr<CMaterial> gtao_blur;
    std::shared_ptr<CMaterial> gtao_color;

    std::shared_ptr<CMaterial> lighting_deferred;
    std::shared_ptr<CMaterial> lighting_indirect;

    RHI::CDescriptorSet::Ref EngineCommonDS;
};

} /* namespace Foreground */
