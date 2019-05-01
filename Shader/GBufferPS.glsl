#version 450
#include "EngineCommon.h"

#define INOUT in
#include "{{VSInterpolantsHeader}}"
#undef INOUT

layout (location=0) out vec4 GBuffer0;
layout (location=1) out vec4 GBuffer1;
layout (location=2) out vec4 GBuffer2;
layout (location=3) out vec4 GBuffer3;

#include "{{MaterialHeader}}"

void main()
{
    GBuffer0 = vec4(Material_GetBaseColor().rgb, 0);
    GBuffer1 = vec4(Interpolants_GetWorldNormal(), 0);
    GBuffer2 = vec4(0);
    GBuffer3 = vec4(0);
}
