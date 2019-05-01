#include "ImGuiRenderer.h"
#include <RHIImGuiBackend.h>

namespace Foreground
{

CImGuiRenderer::CImGuiRenderer(RHI::CRenderPass& renderPass)
{
    RHI::CRHIImGuiBackend::Init(RenderDevice, renderPass.shared_from_this());
}

CImGuiRenderer::~CImGuiRenderer() { RHI::CRHIImGuiBackend::Shutdown(); }

void CImGuiRenderer::Render(RHI::IRenderContext& ctx)
{
    ImGui::Render();
    RHI::CRHIImGuiBackend::RenderDrawData(ImGui::GetDrawData(), ctx);
}

}
