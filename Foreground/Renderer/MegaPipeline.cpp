#include "MegaPipeline.h"
#include "GBufferRenderer.h"
#include "Resources/ResourceManager.h"

#include <RHIImGuiBackend.h>
#include <ShaderModule.h>
#include <fstream>
#include <imgui.h>
#include <iostream>

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
        desc.AddressModeU = RHI::ESamplerAddressMode::Wrap;
        desc.AddressModeV = RHI::ESamplerAddressMode::Wrap;
        desc.AddressModeW = RHI::ESamplerAddressMode::Wrap;
        desc.MaxLod = 4;
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

        RHI::CRHIImGuiBackend::Init(RenderDevice, gtao_visibility->getRenderPass());
    }

    void CMegaPipeline::SetSceneView(std::unique_ptr<CSceneView> sceneView)
    {
        SceneView = std::move(sceneView);
    }

    void CMegaPipeline::Resize() {
        std::cout << "Resizing" << std::endl;

        SwapChain->AutoResize();
        uint32_t w, h;
        SwapChain->GetSize(w, h);

        GBufferPass = nullptr;

        gtao_visibility = nullptr;
        gtao_blur = nullptr;

        CreateGBufferPass(w, h);
        CreateScreenPass();
    }

    void CMegaPipeline::Render()
    {
        // Or render some test image?
        if (!SceneView)
            return;

        if (!SwapChain->AcquireNextImage())
        {
            return;
        }

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

        GlobalConstants constants;
        constants.CameraPos = tc::Vector4(SceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
        constants.ViewMat =
            SceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4().Transpose();
        constants.ProjMat = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Transpose();
        constants.InvProj =
            SceneView->GetCameraNode()->GetCamera()->GetMatrix().Inverse().Transpose();

        gtao_visibility->beginRender(ctx);
        gtao_visibility->setSampler("s", GlobalLinearSampler);
        gtao_visibility->setImageView("t_albedo", GBuffer0);
        gtao_visibility->setImageView("t_normals", GBuffer1);
        gtao_visibility->setImageView("t_depth", GBufferDepth);
        gtao_visibility->setStruct("GlobalConstants", sizeof(GlobalConstants), &constants);
        gtao_visibility->blit2d();
        gtao_visibility->endRender();

        gtao_blur->beginRender(ctx);
        gtao_blur->setSampler("s", GlobalLinearSampler);
        gtao_blur->setImageView("t_ao", gtao_visibility->getRTViews()[0]);
        gtao_blur->blit2d();

        auto* drawData = ImGui::GetDrawData();
        if (drawData)
            RHI::CRHIImGuiBackend::RenderDrawData(drawData, *ctx);

        gtao_blur->endRender();

        SceneView->FrameFinished();

        RHI::CSwapChainPresentInfo info;
        SwapChain->Present(info);
    }

    void CMegaPipeline::BindEngineCommon(RHI::IRenderContext & context)
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
            EFormat::R16G16B16A16_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
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
        normalsViewDesc.Format = EFormat::R16G16B16A16_UNORM;
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

        auto aoImage = RenderDevice->CreateImage2D(
            EFormat::R16G16B16A16_SFLOAT,
            EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
            width, height);

        gtao_visibility = std::shared_ptr<CMaterial>(
            new CMaterial(RenderDevice, "Common/Quad.vert.spv", "GTAO/gtao.frag.spv"));

        gtao_visibility->renderTargets = { { aoImage, EFormat::R16G16B16A16_SFLOAT } };
        gtao_visibility->createPipeline(width, height);
        
        gtao_blur = std::shared_ptr<CMaterial>(
            new CMaterial(RenderDevice, "Common/Quad.vert.spv", "GTAO/blur.frag.spv"));

        gtao_blur->renderTargets = { { fbImage } };
        gtao_blur->createPipeline(width, height);
    }

} /* namespace Foreground */
