#pragma once
#include "RenderConductor.h"

#include <RHIInstance.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4

namespace Foreground
{

// Oversees the entire render process of a frame
//   Maybe subclassed/specialized to support different rendering algorithms
class DeferredConductor : public CRenderConductor
{
private:
    std::shared_ptr<RHI::IImmediateContext> ctx;
    std::shared_ptr<RHI::CDevice> device;

    CRenderView* view;

    std::vector<CPrimitive*> drawList;

    struct GBuffer
    {
        RHI::CImageView::Ref albedoView;
        RHI::CImageView::Ref normalsView;
        RHI::CImageView::Ref depthView;
    } gbuffer;

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 color;
    };

    // Pipelines & resources
    RHI::CSwapChain::Ref swapChain;

    RHI::CPipeline::Ref pso_gbuffer;
    RHI::CRenderPass::Ref gbuffer_pass;

    RHI::CPipeline::Ref pso_screen;
    RHI::CRenderPass::Ref screen_pass;

    RHI::CSampler::Ref gbuffer_sampler;

    void CreateGBufferPass();
    void CreateScreenPass();

public:
    DeferredConductor(std::shared_ptr<RHI::IImmediateContext> ctx,
                      std::shared_ptr<RHI::CDevice> device,
                      RHI::CSwapChain::Ref swapChain);

    void SetMainView(CRenderView* view);
    void SetMainDrawList(std::vector<CPrimitive*> drawList);

    void ProcessAndSubmit();
};

} /* namespace Foreground */
