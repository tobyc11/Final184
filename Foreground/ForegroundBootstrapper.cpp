#include "ForegroundBootstrapper.h"
#include "ForegroundCommon.h"
#include "Renderer/MegaPipeline.h"

namespace Foreground
{

void CForegroundBootstrapper::Init(RHI::CDevice::Ref device)
{
    if (bIsBootstrapped)
        throw std::runtime_error("Foreground is already initialized");
    bIsBootstrapped = true;

    RenderDevice = device;
    RenderQueue = RenderDevice->CreateCommandQueue();
    PipelangContext.SetDevice(RenderDevice);
}

void CForegroundBootstrapper::Shutdown()
{
    if (!bIsBootstrapped)
        throw std::runtime_error("Foreground has not been initialized");
    bIsBootstrapped = false;

    PipelangContext.SetDevice(nullptr);
    RenderDevice->WaitIdle();
    RenderQueue.reset();
    RenderDevice.reset();
}

std::shared_ptr<CMegaPipeline>
CForegroundBootstrapper::CreateRenderPipeline(const RHI::CPresentationSurfaceDesc& surfaceDesc,
                                              EForegroundPipeline kind)
{
    throw "unimplemented";
}

std::shared_ptr<CMegaPipeline>
CForegroundBootstrapper::CreateRenderPipeline(RHI::CSwapChain::Ref swapChain,
                                              EForegroundPipeline kind)
{
    return std::make_shared<CMegaPipeline>(swapChain);
}

} /* namespace Foreground */
