#pragma once
#include "ForegroundAPI.h"
#include <Device.h>

namespace Foreground
{

// This is meant to be a private header internal to foreground

// The entire render engine should use only once device to avoid confusion. After all, there is
// currently no way to share resources across different devices.

extern bool bIsBootstrapped;
extern RHI::CDevice::Ref RenderDevice;

} /* namespace Foreground */
