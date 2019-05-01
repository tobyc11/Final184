#pragma once
#include "ForegroundCommon.h"
#include <RenderContext.h>
#include <RenderPass.h>

namespace Foreground
{

// The renderer responsible for rendering ImGui into a color pass
// Renderers don't hold references to the render pass because they are intended to be compatible
// with other render passes that have identical attachments
class CImGuiRenderer
{
public:
    CImGuiRenderer(RHI::CRenderPass& renderPass);
    ~CImGuiRenderer();

    // Render is called with a compatible render pass already bound
    void Render(RHI::IRenderContext& ctx);
};

} /* namespace Foreground */
