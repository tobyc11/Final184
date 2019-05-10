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

struct GlobalConstants
{
    tc::Vector4 CameraPos;
    tc::Matrix4 ViewMat;
    tc::Matrix4 ProjMat;
    tc::Matrix4 InvProj;
};

class CMegaPipeline
{
public:
    explicit CMegaPipeline(RHI::CSwapChain::Ref swapChain);

    void SetSceneView(std::unique_ptr<CSceneView> sceneView,
                      std::unique_ptr<CSceneView> shadowView);

    void Resize();
    void Render();

    // Called by the renderers to set global constants
    void UpdateEngineCommon();
    void BindEngineCommon(RHI::IRenderContext& context);

    void UpdateEngineCommonShadow();
    void BindEngineCommonShadow(RHI::IRenderContext& context);

    RHI::CImageView::Ref getVoxelsImageView() const { return VoxelBuffer; };

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

    RHI::CImage::Ref VoxelImage;

    RHI::CImageView::Ref GBuffer0;
    RHI::CImageView::Ref GBuffer1;
    RHI::CImageView::Ref GBuffer2;
    // RHI::CImageView::Ref GBuffer3;
    RHI::CImageView::Ref GBufferDepth;
    RHI::CImageView::Ref ShadowDepth;
    RHI::CImageView::Ref VoxelBuffer;
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

    RHI::CDescriptorSet::Ref EngineCommonDS;
    RHI::CDescriptorSet::Ref EngineCommonShadowDS;
};

} /* namespace Foreground */
