#include "ForegroundCommon.h"
#include "Shader/ShaderCache.h"

namespace Foreground
{

bool bIsBootstrapped = false;
RHI::CDevice::Ref RenderDevice;
CShaderFactory GlobalShaderFactory;

} /* namespace Foreground */
