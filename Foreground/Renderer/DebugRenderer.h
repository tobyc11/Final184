#pragma once
#include <RenderPass.h>

namespace Foreground
{

// Renders debug primitives
class CDebugRenderer
{
public:
    CDebugRenderer(RHI::CRenderPass::Ref parentPass);
};

} /* namespace Foreground */
