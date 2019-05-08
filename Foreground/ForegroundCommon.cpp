#include "ForegroundCommon.h"

namespace Foreground
{

bool bIsBootstrapped = false;
RHI::CDevice::Ref RenderDevice;
RHI::CCommandQueue::Ref RenderQueue;
Pl::CPipelangContext PipelangContext;

} /* namespace Foreground */
