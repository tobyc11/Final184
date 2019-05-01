#include "MegaPipeline.h"
#include "GBufferRenderer.h"
#include "Resources/ResourceManager.h"
#include <RHIImGuiBackend.h>
#include <ShaderModule.h>
#include <fstream>
#include <imgui.h>

namespace Foreground
{

static RHI::CShaderModule::Ref LoadSPIRV(RHI::CDevice::Ref device, const std::string& path)
{
    std::string filePath = CResourceManager::Get().FindShader(path);
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return device->CreateShaderModule(buffer.size(), buffer.data());
    return {};
}

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

    RHI::CRHIImGuiBackend::Init(RenderDevice, ScreenPass);

    {
        RHI::CPipelineDesc desc;
        desc.VS = LoadSPIRV(RenderDevice, "Quad.vert.spv");
        desc.PS = LoadSPIRV(RenderDevice, "deferred.frag.spv");
        desc.RasterizerState.CullMode = RHI::ECullModeFlags::None;
        desc.DepthStencilState.DepthEnable = false;
        desc.RenderPass = ScreenPass;
        desc.Subpass = 0;
        BlitPipeline = RenderDevice->CreatePipeline(desc);
    }
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

    SwapChain->AcquireNextImage();

    // Does culling and stuff
    SceneView->GetCameraNode()->GetScene()->UpdateAccelStructure();
    SceneView->PrepareToRender();
    auto ctx = RenderDevice->GetImmediateContext();
    ctx->BeginRenderPass(*GBufferPass,
                         { RHI::CClearValue(0.0f, 0.0f, 0.0f, 0.0f),
                           RHI::CClearValue(0.0f, 0.0f, 0.0f, 0.0f), RHI::CClearValue(1.0f, 0) });
    GBufferRenderer.RenderList(*ctx, SceneView->GetVisiblePrimModelMatrix(),
                               SceneView->GetVisiblePrimitiveList());
    ctx->EndRenderPass();

    ctx->BeginRenderPass(*ScreenPass, { RHI::CClearValue(0.0f, 0.0f, 0.0f, 0.0f) });
    ctx->BindPipeline(*BlitPipeline);
    BindEngineCommon(*ctx);
    ctx->BindSampler(*GlobalLinearSampler, 1, 0, 0);
    ctx->BindImageView(*GBuffer1, 1, 1, 0);
    ctx->BindImageView(*GBufferDepth, 1, 2, 0);
    ctx->Draw(3, 1, 0, 0);

    auto* drawData = ImGui::GetDrawData();
    if (drawData)
        RHI::CRHIImGuiBackend::RenderDrawData(drawData, *ctx);
    ctx->EndRenderPass();
    SceneView->FrameFinished();

    RHI::CSwapChainPresentInfo info;
    SwapChain->Present(info);
}

void CMegaPipeline::BindEngineCommon(RHI::IRenderContext& context)
{
    context.BindSampler(*GlobalNiceSampler, 0, 0, 0);
    context.BindSampler(*GlobalLinearSampler, 0, 1, 0);
    context.BindSampler(*GlobalNearestSampler, 0, 2, 0);

    GlobalConstants constants;
    constants.CameraPos = tc::Vector4(SceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
    constants.ViewMat =
        SceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4().Transpose();
    constants.ProjMat = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Transpose();
    constants.InvProj = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Inverse().Transpose();
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
