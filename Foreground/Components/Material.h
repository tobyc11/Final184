#pragma once

#include "Event.h"
#include "GameObject.h"

#include <unordered_map>

#include <Pipeline.h>
#include <Resources.h>
#include <Sampler.h>
#include <RenderContext.h>

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
    RHI::CImageView::Ref image;
    RHI::EAttachmentLoadOp loadOp = RHI::EAttachmentLoadOp::Clear;
    RHI::EAttachmentStoreOp storeOp = RHI::EAttachmentStoreOp::Store;
    RHI::CClearValue clearValue = { 0.0, 0.0, 0.0, 0.0 };
    bool isDepthStencil = false;
};

class CMaterial : public CGameObject
{
private:
    RHI::CRenderPass::Ref renderPass;
    RHI::CPipeline::Ref pipeline;

    std::unordered_map<std::string, CMaterialProperties> properties;
    std::unordered_map<std::string, CMaterialAttributes> attributes;

public:
    std::vector<CRenderTarget> renderTargets;

    CMaterial(const std::string& VS_file, const std::string& PS_file);

    void setSampler(std::string id, RHI::CSampler::Ref obj);
    void setImageView(std::string id, RHI::CImageView::Ref obj);

    void beginRender() const;
    void endRender() const;

    void drawObject() const {}; // eh... TODO
    void blit2d() const;
};

}