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
    , ZOnlyRenderer(this)
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
    ZOnlyRenderer.SetRenderPass(ZOnlyPass);

    RHI::CRHIImGuiBackend::Init(RenderDevice, gtao_color->getRenderPass());

    PipelangContext.CreateLibrary("Internal").Parse();
}

void CMegaPipeline::SetSceneView(std::unique_ptr<CSceneView> sceneView,
                                 std::unique_ptr<CSceneView> shadowView)
{
    SceneView = std::move(sceneView);
    ShadowSceneView = std::move(shadowView);
}

void CMegaPipeline::Resize()
{
    std::cout << "Resizing" << std::endl;

    SwapChain->AutoResize();
    uint32_t w, h;
    SwapChain->GetSize(w, h);

    GBufferPass = nullptr;

    gtao_visibility = nullptr;
    gtao_blur = nullptr;
    gtao_color = nullptr;
    lighting_deferred = nullptr;

    CreateGBufferPass(w, h);
    CreateScreenPass();
}

struct alignas(16) PerLightConstants
{
    tc::Color color;
    tc::Vector3 position;
};

struct alignas(16) LightLists
{
    PerLightConstants lights[100];
    int numLights;
};

void getAllLights(LightLists* lightsPoint, LightLists* lightsDirectioanl, CSceneNode* root)
{
    tc::Vector3 position = root->GetWorldPosition();
    tc::Vector3 direction = root->GetWorldTransform() * tc::Vector4(0.0, 0.0, -1.0, 0.0);
    for (auto L : root->GetLights())
    {
        if (L->getType() == ELightType::Point)
        {
            lightsPoint->lights[lightsPoint->numLights] = { L->getLuminance(), position };
            lightsPoint->numLights++;
        }
        if (L->getType() == ELightType::Directional)
        {
            lightsDirectioanl->lights[lightsDirectioanl->numLights] = { L->getLuminance(),
                                                                        direction };
            lightsDirectioanl->numLights++;
        }
    }

    for (CSceneNode* child : root->GetChildren())
    {
        getAllLights(lightsPoint, lightsDirectioanl, child);
    }
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

    ShadowSceneView->PrepareToRender();

    auto cmdList = RenderQueue->CreateCommandList();
    cmdList->Enqueue();

    CSceneNode* root = SceneView->GetCameraNode()->GetScene()->GetRootNode();
    LightLists pointLightLists, directionalLightLists;
    pointLightLists.numLights = 0;
    directionalLightLists.numLights = 0;
    getAllLights(&pointLightLists, &directionalLightLists, root);

    auto passCtx = cmdList->CreateParallelRenderContext(GBufferPass,
                                                        { RHI::CClearValue(0.0f, 0.0f, 0.0f, 0.0f),
                                                          RHI::CClearValue(0.0f, 0.0f, 0.0f, 0.0f),
                                                          RHI::CClearValue(1.0f, 0) });
    auto ctx = passCtx->CreateRenderContext(0);
    GBufferRenderer.RenderList(*ctx, SceneView->GetVisiblePrimModelMatrix(),
                               SceneView->GetVisiblePrimitiveList());
    ctx->FinishRecording();
    passCtx->FinishRecording();

    passCtx = cmdList->CreateParallelRenderContext(ZOnlyPass,
                                                        { RHI::CClearValue(1.0f, 0) });
    ctx = passCtx->CreateRenderContext(0);
    ZOnlyRenderer.RenderList(*ctx, ShadowSceneView->GetVisiblePrimModelMatrix(),
                             ShadowSceneView->GetVisiblePrimitiveList());
    ctx->FinishRecording();
    passCtx->FinishRecording();

    GlobalConstants constants;
    constants.CameraPos = tc::Vector4(SceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
    constants.ViewMat =
        SceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4().Transpose();
    constants.ProjMat = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Transpose();
    constants.InvProj = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Inverse().Transpose();

    gtao_visibility->beginRender(cmdList);
    gtao_visibility->setSampler("s", GlobalLinearSampler);
    gtao_visibility->setImageView("t_albedo", GBuffer0);
    gtao_visibility->setImageView("t_normals", GBuffer1);
    gtao_visibility->setImageView("t_depth", GBufferDepth);
    gtao_visibility->setStruct("GlobalConstants", sizeof(GlobalConstants), &constants);
    gtao_visibility->blit2d();
    gtao_visibility->endRender();

    gtao_blur->beginRender(cmdList);
    gtao_blur->setSampler("s", GlobalLinearSampler);
    gtao_blur->setImageView("t_ao", gtao_visibility->getRTViews()[0]);
    gtao_blur->blit2d();
    gtao_blur->endRender();

    lighting_deferred->beginRender(cmdList);
    lighting_deferred->setSampler("s", GlobalLinearSampler);
    lighting_deferred->setImageView("t_albedo", GBuffer0);
    lighting_deferred->setImageView("t_normals", GBuffer1);
    lighting_deferred->setImageView("t_depth", GBufferDepth);
    lighting_deferred->setStruct("GlobalConstants", sizeof(GlobalConstants), &constants);
    lighting_deferred->setStruct("pointLights", sizeof(LightLists), &pointLightLists);
    lighting_deferred->setStruct("directionalLights", sizeof(LightLists), &directionalLightLists);
    lighting_deferred->blit2d();
    lighting_deferred->endRender();

    gtao_color->beginRender(cmdList);
    gtao_color->setSampler("s", GlobalLinearSampler);
    gtao_color->setImageView("t_albedo", GBuffer0);
    gtao_color->setImageView("t_ao", gtao_blur->getRTViews()[0]);
    gtao_color->setImageView("t_lighting", lighting_deferred->getRTViews()[0]);
    gtao_color->setImageView("t_shadow", ShadowDepth);
    gtao_color->blit2d();

    auto* drawData = ImGui::GetDrawData();
    if (drawData)
        RHI::CRHIImGuiBackend::RenderDrawData(drawData, *gtao_color->getContext());

    gtao_color->endRender();
    cmdList->Commit();

    SceneView->FrameFinished();
    ShadowSceneView->FrameFinished();

    RHI::CSwapChainPresentInfo info;
    SwapChain->Present(info);
}

void CMegaPipeline::UpdateEngineCommon()
{
    if (!EngineCommonDS)
    {
        auto& lib = PipelangContext.GetLibrary("Internal");
        auto& pb = lib.GetParameterBlock("EngineCommon");
        EngineCommonDS = pb.CreateDescriptorSet();
    }

    auto& lib = PipelangContext.GetLibrary("Internal");
    auto& pb = lib.GetParameterBlock("EngineCommon");
    pb.BindSampler(EngineCommonDS, GlobalNiceSampler, "GlobalNiceSampler");
    pb.BindSampler(EngineCommonDS, GlobalLinearSampler, "GlobalLinearSampler");
    pb.BindSampler(EngineCommonDS, GlobalNearestSampler, "GlobalNearestSampler");

    GlobalConstants constants;
    constants.CameraPos = tc::Vector4(SceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
    constants.ViewMat =
        SceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4().Transpose();
    constants.ProjMat = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Transpose();
    constants.InvProj = SceneView->GetCameraNode()->GetCamera()->GetMatrix().Inverse().Transpose();
    pb.BindConstants(EngineCommonDS, &constants, sizeof(GlobalConstants), "GlobalConstants");
}

void CMegaPipeline::BindEngineCommon(RHI::IRenderContext& context)
{
    UpdateEngineCommon();
    context.BindRenderDescriptorSet(0, *EngineCommonDS);
}


void CMegaPipeline::UpdateEngineCommonShadow()
{
    if (!EngineCommonShadowDS)
    {
        auto& lib = PipelangContext.GetLibrary("Internal");
        auto& pb = lib.GetParameterBlock("EngineCommon");
        EngineCommonShadowDS = pb.CreateDescriptorSet();
    }

    auto& lib = PipelangContext.GetLibrary("Internal");
    auto& pb = lib.GetParameterBlock("EngineCommon");
    pb.BindSampler(EngineCommonShadowDS, GlobalNiceSampler, "GlobalNiceSampler");
    pb.BindSampler(EngineCommonShadowDS, GlobalLinearSampler, "GlobalLinearSampler");
    pb.BindSampler(EngineCommonShadowDS, GlobalNearestSampler, "GlobalNearestSampler");

    GlobalConstants constants;
    constants.CameraPos = tc::Vector4(ShadowSceneView->GetCameraNode()->GetWorldPosition(), 1.0f);
    constants.ViewMat =
        ShadowSceneView->GetCameraNode()->GetWorldTransform().Inverse().ToMatrix4().Transpose();
    constants.ProjMat = ShadowSceneView->GetCameraNode()->GetCamera()->GetMatrix().Transpose();
    constants.InvProj =
        ShadowSceneView->GetCameraNode()->GetCamera()->GetMatrix().Inverse().Transpose();
    pb.BindConstants(EngineCommonShadowDS, &constants, sizeof(GlobalConstants), "GlobalConstants");
}

void CMegaPipeline::BindEngineCommonShadow(RHI::IRenderContext& context)
{
    UpdateEngineCommonShadow();
    context.BindRenderDescriptorSet(0, *EngineCommonShadowDS);
}

void CMegaPipeline::CreateRenderPasses()
{
    uint32_t w, h;
    SwapChain->GetSize(w, h);
    CreateShadowBufferPass();
    CreateGBufferPass(w, h);
    CreateScreenPass();
}

void CMegaPipeline::CreateShadowBufferPass()
{
    using namespace RHI;

    auto depthImage = RenderDevice->CreateImage2D(
        EFormat::D32_SFLOAT_S8_UINT, EImageUsageFlags::DepthStencil | EImageUsageFlags::Sampled,
        2048, 2048);

    CImageViewDesc depthViewDesc;
    depthViewDesc.Format = EFormat::D32_SFLOAT_S8_UINT;
    depthViewDesc.Type = EImageViewType::View2D;
    depthViewDesc.DepthStencilAspect = EDepthStencilAspectFlags::Depth;
    depthViewDesc.Range.Set(0, 1, 0, 1);
    ShadowDepth = RenderDevice->CreateImageView(depthViewDesc, depthImage);

    CRenderPassDesc rpDesc;
    rpDesc.AddAttachment(ShadowDepth, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.NextSubpass().SetDepthStencilAttachment(0);
    rpDesc.Width = 2048;
    rpDesc.Height = 2048;
    rpDesc.Layers = 1;
    ZOnlyPass = RenderDevice->CreateRenderPass(rpDesc);
}

void CMegaPipeline::CreateGBufferPass(uint32_t width, uint32_t height)
{
    using namespace RHI;

    auto albedoImage = RenderDevice->CreateImage2D(
        EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
        height);
    auto normalsImage = RenderDevice->CreateImage2D(
        EFormat::R16G16B16A16_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
        width, height);
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
        EFormat::R16G16B16A16_SFLOAT, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
        width, height);
    auto aoImage2 = RenderDevice->CreateImage2D(
        EFormat::R16G16B16A16_SFLOAT, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
        width, height);

    auto lightingImage = RenderDevice->CreateImage2D(
        EFormat::R16G16B16A16_SFLOAT, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled,
        width, height);

    gtao_visibility = std::shared_ptr<CMaterial>(
        new CMaterial(RenderDevice, "Common/Quad.vert.spv", "GTAO/gtao.frag.spv"));

    gtao_visibility->renderTargets = { { aoImage, EFormat::R16G16B16A16_SFLOAT } };
    gtao_visibility->createPipeline(width, height);

    gtao_blur = std::shared_ptr<CMaterial>(
        new CMaterial(RenderDevice, "Common/Quad.vert.spv", "GTAO/blur.frag.spv"));

    gtao_blur->renderTargets = { { aoImage2, EFormat::R16G16B16A16_SFLOAT } };
    gtao_blur->createPipeline(width, height);

    gtao_color = std::shared_ptr<CMaterial>(
        new CMaterial(RenderDevice, "Common/Quad.vert.spv", "GTAO/color.frag.spv"));

    gtao_color->renderTargets = { { fbImage } };
    gtao_color->createPipeline(width, height);

    lighting_deferred = std::shared_ptr<CMaterial>(
        new CMaterial(RenderDevice, "Common/Quad.vert.spv", "Lighting/aggregateLights.frag.spv"));

    lighting_deferred->renderTargets = { { lightingImage, EFormat::R16G16B16A16_SFLOAT } };
    lighting_deferred->createPipeline(width, height);
}

} /* namespace Foreground */
