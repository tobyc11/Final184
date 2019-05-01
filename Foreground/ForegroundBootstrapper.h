#pragma once
#include <Device.h>
#include <PresentationSurfaceDesc.h>

namespace Foreground
{

class CMegaPipeline;

enum class EForegroundPipeline
{
    Mega
};

class CForegroundBootstrapper
{
public:
    static void Init(RHI::CDevice::Ref device);
    static void Shutdown();

    // Create a render pass and a swap chain from a presentation surface
    static std::shared_ptr<CMegaPipeline>
    CreateRenderPipeline(const RHI::CPresentationSurfaceDesc& surfaceDesc,
                         EForegroundPipeline kind);

    // Create a render pipeline with an existing swap chain
    static std::shared_ptr<CMegaPipeline> CreateRenderPipeline(const RHI::CSwapChain& swapChain,
                                                               EForegroundPipeline kind);
};

} /* namespace Foreground */
