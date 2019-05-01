#include "MegaPipeline.h"

namespace Foreground
{

CMegaPipeline::CMegaPipeline(RHI::CSwapChain::Ref swapChain) {}

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
