#include "MegaPipeline.h"
#include "GBufferRenderer.h"

namespace Foreground
{

CMegaPipeline::CMegaPipeline(RHI::CSwapChain::Ref swapChain)
    : SwapChain(swapChain)
    , GBufferRenderer(this)
{
    RHI::CSamplerDesc desc;
    GlobalLinearSampler = RenderDevice->CreateSampler(desc);
    desc.MinFilter = RHI::EFilter::Nearest;
    desc.MagFilter = RHI::EFilter::Nearest;
    desc.MipmapMode = RHI::ESamplerMipmapMode::Nearest;
    GlobalNearestSampler = RenderDevice->CreateSampler(desc);
    desc.AnisotropyEnable = true;
    desc.MaxAnisotropy = 16.0f;
    GlobalNiceSampler = RenderDevice->CreateSampler(desc);

    CreateRenderPasses();
    GBufferRenderer.SetRenderPass(GBufferPass);
}

void CMegaPipeline::SetSceneView(std::unique_ptr<CSceneView> sceneView)
{
    SceneView = std::move(sceneView);
}

void CMegaPipeline::Render()
{
    // Or render some test image?
    if (!SceneView)
        return;

    // Does culling and stuff
    SceneView->PrepareToRender();
    GBufferRenderer.RenderList(*RenderDevice->GetImmediateContext(),
                               SceneView->GetVisiblePrimModelMatrix(),
                               SceneView->GetVisiblePrimitiveList());
    SceneView->FrameFinished();
}

void CMegaPipeline::BindEngineCommon(RHI::IRenderContext& context)
{
    context.BindSampler(*GlobalNiceSampler, 0, 0, 0);
    context.BindSampler(*GlobalLinearSampler, 0, 1, 0);
    context.BindSampler(*GlobalNearestSampler, 0, 2, 0);

    GlobalConstants constants;
    constants.CameraPos = tc::Vector4(SceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
    constants.ViewMat = SceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4();
    constants.ProjMat = SceneView->GetCameraNode()->GetCamera()->GetMatrix();
    context.BindConstants(&constants, sizeof(GlobalConstants), 0, 3, 0);
}

void CMegaPipeline::CreateRenderPasses()
{
    uint32_t w, h;
    SwapChain->GetSize(w, h);
    CreateGBufferPass(w, h);
    CreateScreenPass();
}

void CMegaPipeline::CreateGBufferPass(uint32_t width, uint32_t height)
{
    using namespace RHI;

    auto albedoImage = RenderDevice->CreateImage2D(
        EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
        height);
    auto normalsImage = RenderDevice->CreateImage2D(
        EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
        height);
    auto depthImage = RenderDevice->CreateImage2D(
        EFormat::D32_SFLOAT_S8_UINT, EImageUsageFlags::DepthStencil | EImageUsageFlags::Sampled,
        width, height);

    CImageViewDesc albedoViewDesc;
    albedoViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    albedoViewDesc.Type = EImageViewType::View2D;
    albedoViewDesc.Range.Set(0, 1, 0, 1);
    GBuffer0 = RenderDevice->CreateImageView(albedoViewDesc, albedoImage);

    CImageViewDesc normalsViewDesc;
    normalsViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    normalsViewDesc.Type = EImageViewType::View2D;
    normalsViewDesc.Range.Set(0, 1, 0, 1);
    GBuffer1 = RenderDevice->CreateImageView(normalsViewDesc, normalsImage);

    CImageViewDesc depthViewDesc;
    depthViewDesc.Format = EFormat::D32_SFLOAT_S8_UINT;
    depthViewDesc.Type = EImageViewType::View2D;
    depthViewDesc.DepthStencilAspect = EDepthStencilAspectFlags::Depth;
    depthViewDesc.Range.Set(0, 1, 0, 1);
    GBufferDepth = RenderDevice->CreateImageView(depthViewDesc, depthImage);

    CRenderPassDesc rpDesc;
    rpDesc.AddAttachment(GBuffer0, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.AddAttachment(GBuffer1, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.AddAttachment(GBufferDepth, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.NextSubpass().AddColorAttachment(0).AddColorAttachment(1).SetDepthStencilAttachment(2);
    rpDesc.Width = width;
    rpDesc.Height = height;
    rpDesc.Layers = 1;
    GBufferPass = RenderDevice->CreateRenderPass(rpDesc);
}

void CMegaPipeline::CreateScreenPass()
{
    using namespace RHI;

    auto fbImage = SwapChain->GetImage();
    uint32_t width, height;
    SwapChain->GetSize(width, height);

    CImageViewDesc fbViewDesc;
    fbViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    fbViewDesc.Type = EImageViewType::View2D;
    fbViewDesc.Range.Set(0, 1, 0, 1);
    auto fbView = RenderDevice->CreateImageView(fbViewDesc, fbImage);

    CRenderPassDesc rpDesc;
    rpDesc.AddAttachment(fbView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.Subpasses.resize(1);
    rpDesc.Subpasses[0].AddColorAttachment(0);
    SwapChain->GetSize(rpDesc.Width, rpDesc.Height);
    rpDesc.Layers = 1;
    ScreenPass = RenderDevice->CreateRenderPass(rpDesc);
}

} /* namespace Foreground */
