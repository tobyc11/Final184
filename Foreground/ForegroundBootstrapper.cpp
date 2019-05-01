#include "ForegroundBootstrapper.h"
#include "ForegroundCommon.h"

namespace Foreground
{

void CForegroundBootstrapper::Init(RHI::CDevice::Ref device)
{
    if (bIsBootstrapped)
        throw std::runtime_error("Foreground is already initialized");
    bIsBootstrapped = true;

    RenderDevice = device;
}

void CForegroundBootstrapper::Shutdown()
{
    if (!bIsBootstrapped)
        throw std::runtime_error("Foreground has not been initialized");
    bIsBootstrapped = false;

    RenderDevice.reset();
}

std::shared_ptr<CMegaPipeline>
CForegroundBootstrapper::CreateRenderPipeline(const RHI::CPresentationSurfaceDesc& surfaceDesc,
                                              EForegroundPipeline kind)
{
    throw "unimplemented";
}

} /* namespace Foreground */
