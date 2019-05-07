#pragma once

#include "Event.h"
#include "GameObject.h"

#include <unordered_map>

#include <Device.h>
#include <Pipeline.h>
#include <Resources.h>
#include <Sampler.h>
#include <RenderContext.h>
#include <ShaderModule.h>

namespace Foreground
{

// Uniforms data
struct CMaterialProperties
{
    int set, binding;
};

// Per-vertex (instance) data
// TODO: add this
struct CMaterialAttributes
{
    int location;
};

struct CRenderTarget
{
    RHI::CImage::Ref image;
    RHI::EFormat format = RHI::EFormat::R8G8B8A8_UNORM;
    RHI::CClearValue clearValue = { 0.0, 0.0, 0.0, 0.0 };
    RHI::EAttachmentLoadOp loadOp = RHI::EAttachmentLoadOp::Clear;
    RHI::EAttachmentStoreOp storeOp = RHI::EAttachmentStoreOp::Store;
    bool isDepthStencil = false;
};

class CMaterial : public CGameObject
{
private:
    RHI::CRenderPass::Ref renderPass;
    RHI::CPipeline::Ref pipeline;
    RHI::CShaderModule::Ref VS, PS;

    std::unordered_map<std::string, RHI::CPipelineResource> resources;

    std::vector<RHI::CImageView::Ref> renderTargetViews;

    RHI::CDevice::Ref device;

    RHI::IImmediateContext::Ref ctx = nullptr;

    std::vector<RHI::CClearValue> clearValues;

public:
    std::vector<CRenderTarget> renderTargets;

    CMaterial(RHI::CDevice::Ref device, const std::string& VS_file, const std::string& PS_file);
    ~CMaterial();

    void createPipeline(int w, int h);

    void setSampler(std::string id, RHI::CSampler::Ref obj);
    void setImageView(std::string id, RHI::CImageView::Ref obj);
    void setStruct(std::string id, size_t size, const void* obj);

    const std::vector<RHI::CImageView::Ref>& getRTViews() const;

    void beginRender(RHI::IImmediateContext::Ref ctx);
    void endRender();

    RHI::CRenderPass::Ref getRenderPass() const { return renderPass; }
    RHI::CPipeline::Ref getPipeline() const { return pipeline; }

    void drawObject() const {}; // eh... TODO
    void blit2d() const;
};

}