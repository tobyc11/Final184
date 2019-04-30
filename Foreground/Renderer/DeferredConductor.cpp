#include "DeferredConductor.h"

#include <fstream>

#include <Resources/ResourceManager.h>

using namespace RHI;

namespace Foreground
{

    static CShaderModule::Ref LoadSPIRV(CDevice::Ref device, const std::string& name)
    {
        std::ifstream file(CResourceManager::Get().FindShader(name).c_str(),
                           std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
            return device->CreateShaderModule(buffer.size(), buffer.data());
        return {};
    }

    DeferredConductor::DeferredConductor(std::shared_ptr<IImmediateContext> ctx,
                                         std::shared_ptr<CDevice> device,
                                         CSwapChain::Ref swapChain)
    {
        this->ctx = ctx;
        this->device = device;
        this->swapChain = swapChain;

        CreateGBufferPass();
        CreateScreenPass();

        {
            CPipelineDesc pipelineDesc;
            pipelineDesc.VS = LoadSPIRV(device, "gbuffers.vert.spv");
            pipelineDesc.PS = LoadSPIRV(device, "gbuffers.frag.spv");
            pipelineDesc.RasterizerState.CullMode = ECullModeFlags::None;
            pipelineDesc.RenderPass = gbuffer_pass;

            CVertexInputBindingDesc vertInputBinding = { 0, sizeof(Vertex), false };

            pipelineDesc.VertexAttributes = {
                { 0, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, pos), 0 },
                { 1, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, normal), 0 },
                { 2, EFormat::R32G32_SFLOAT, offsetof(Vertex, uv), 0 },
                { 3, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, color), 0 }
            };
            pipelineDesc.VertexBindings = { vertInputBinding };

            pso_gbuffer = device->CreatePipeline(pipelineDesc);
        }

        {
            CPipelineDesc pipelineDesc;
            pipelineDesc.VS = LoadSPIRV(device, "deferred.vert.spv");
            pipelineDesc.PS = LoadSPIRV(device, "deferred.frag.spv");
            pipelineDesc.RasterizerState.CullMode = ECullModeFlags::None;
            pipelineDesc.RenderPass = screen_pass;

            pso_screen = device->CreatePipeline(pipelineDesc);
        }

        CSamplerDesc samplerDesc;
        gbuffer_sampler = device->CreateSampler(samplerDesc);
    }

    void DeferredConductor::CreateGBufferPass()
    {
        uint32_t width, height;
        swapChain->GetSize(width, height);

        auto albedoImage = device->CreateImage2D(
            EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
            width, height);
        auto normalsImage = device->CreateImage2D(
            EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
            width, height);
        auto depthImage = device->CreateImage2D(
            EFormat::D32_SFLOAT_S8_UINT, EImageUsageFlags::DepthStencil | EImageUsageFlags::Sampled,
            width, height);

        CImageViewDesc albedoViewDesc;
        albedoViewDesc.Format = EFormat::R8G8B8A8_UNORM;
        albedoViewDesc.Type = EImageViewType::View2D;
        albedoViewDesc.Range.Set(0, 1, 0, 1);
        auto albedoView = device->CreateImageView(albedoViewDesc, albedoImage);
        gbuffer.albedoView = albedoView;

        CImageViewDesc normalsViewDesc;
        normalsViewDesc.Format = EFormat::R8G8B8A8_UNORM;
        normalsViewDesc.Type = EImageViewType::View2D;
        normalsViewDesc.Range.Set(0, 1, 0, 1);
        auto normalsView = device->CreateImageView(normalsViewDesc, normalsImage);
        gbuffer.normalsView = normalsView;

        CImageViewDesc depthViewDesc;
        depthViewDesc.Format = EFormat::D32_SFLOAT_S8_UINT;
        depthViewDesc.Type = EImageViewType::View2D;
        depthViewDesc.DepthStencilAspect = EDepthStencilAspectFlags::Depth;
        depthViewDesc.Range.Set(0, 1, 0, 1);
        auto depthView = device->CreateImageView(depthViewDesc, depthImage);
        gbuffer.depthView = depthView;

        CRenderPassDesc rpDesc;
        rpDesc.AddAttachment(albedoView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
        rpDesc.AddAttachment(normalsView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
        rpDesc.AddAttachment(depthView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
        rpDesc.NextSubpass().AddColorAttachment(0).AddColorAttachment(1).SetDepthStencilAttachment(
            2);
        swapChain->GetSize(rpDesc.Width, rpDesc.Height);
        rpDesc.Layers = 1;
        
        gbuffer_pass = device->CreateRenderPass(rpDesc);
    }

    void DeferredConductor::CreateScreenPass()
    {
        auto fbImage = swapChain->GetImage();
        uint32_t width, height;
        swapChain->GetSize(width, height);

        CImageViewDesc fbViewDesc;
        fbViewDesc.Format = EFormat::R8G8B8A8_UNORM;
        fbViewDesc.Type = EImageViewType::View2D;
        fbViewDesc.Range.Set(0, 1, 0, 1);
        auto fbView = device->CreateImageView(fbViewDesc, fbImage);

        CRenderPassDesc rpDesc;
        rpDesc.AddAttachment(fbView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
        rpDesc.Subpasses.resize(1);
        rpDesc.Subpasses[0].AddColorAttachment(0);
        swapChain->GetSize(rpDesc.Width, rpDesc.Height);
        rpDesc.Layers = 1;
        
        screen_pass = device->CreateRenderPass(rpDesc);
    }

    void DeferredConductor::SetMainView(CRenderView* view) {
        this->view = view;
    }

    void DeferredConductor::SetMainDrawList(std::vector<CPrimitive*> drawList) {
        this->drawList = drawList;
    }

    void DeferredConductor::ProcessAndSubmit() {
        ctx->BeginRenderPass(*gbuffer_pass,
                             { CClearValue(0.2f, 0.3f, 0.4f, 0.0f),
                               CClearValue(0.0f, 0.0f, 0.0f, 0.0f), CClearValue(1.0f, 0) });
        ctx->BindPipeline(*pso_gbuffer);
        //ctx->BindBuffer(*ubo, 0, sizeof(ShadersUniform), 0, 1, 0);
        ctx->EndRenderPass();

        ctx->BeginRenderPass(*screen_pass, { CClearValue(0.0f, 0.0f, 0.0f, 0.0f) });
        ctx->BindPipeline(*pso_screen);
        //ctx->BindBuffer(*ubo, 0, sizeof(ShadersUniform), 1, 0, 0);
        ctx->BindSampler(*gbuffer_sampler, 0, 0, 0);
        ctx->BindImageView(*gbuffer.albedoView, 0, 1, 0);
        ctx->BindImageView(*gbuffer.normalsView, 0, 2, 0);
        ctx->BindImageView(*gbuffer.depthView, 0, 3, 0);
        ctx->Draw(6, 1, 0, 0);

        ctx->EndRenderPass();
    }

}