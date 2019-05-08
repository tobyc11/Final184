#pragma once

#include "Event.h"
#include "GameObject.h"

#include <unordered_map>

#include <Device.h>
#include <Pipeline.h>
#include <RenderContext.h>
#include <Resources.h>
#include <Sampler.h>
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

struct CMaterialNamedAttribute
{
    std::string id;
    RHI::EFormat format; 
    size_t offset;
    std::string buffer_name;
};

class CMaterial : public CGameObject
{
private:
    RHI::CRenderPass::Ref renderPass;
    RHI::CManagedPipeline::Ref pipeline;
    RHI::CShaderModule::Ref VS, PS;
    std::vector<RHI::CDescriptorSet::Ref> DescriptorSets;

    std::unordered_map<std::string, RHI::CPipelineResource> resources;

    std::vector<RHI::CImageView::Ref> renderTargetViews;

    RHI::CDevice::Ref device;

    RHI::IParallelRenderContext::Ref PassCtx;
    RHI::IRenderContext::Ref ctx = nullptr;

    std::vector<RHI::CClearValue> clearValues;

    std::unordered_map<std::string, RHI::CVertexInputBindingDesc> inputBuffers;
    std::unordered_map<std::string, CMaterialNamedAttribute> vertexAttributes;

public:
    std::vector<CRenderTarget> renderTargets;

    CMaterial(RHI::CDevice::Ref device, const std::string& VS_file, const std::string& PS_file);
    ~CMaterial();

    void createPipeline(int w, int h);

    uint32_t getInputBufferBinding(std::string name) const;

    void setAttribute(std::string id, RHI::EFormat format, size_t offset, std::string buffer_name);

    void setSampler(const std::string& id, RHI::CSampler::Ref obj);
    void setImageView(const std::string& id, RHI::CImageView::Ref obj);
    void setStruct(const std::string& id, size_t size, const void* obj);

    void setBuffer(const std::string& id, RHI::CBuffer& obj, size_t offset = 0);

    const std::vector<RHI::CImageView::Ref>& getRTViews() const;

    void beginRender(const RHI::CCommandList::Ref& ctx);
    const RHI::IRenderContext::Ref& getContext() const;
    void endRender();

    RHI::CRenderPass::Ref getRenderPass() const { return renderPass; }
    RHI::CPipeline::Ref getPipeline() const { return pipeline->Get(); }

    void drawObject() const {}; // eh... TODO
    void blit2d() const;
};

}