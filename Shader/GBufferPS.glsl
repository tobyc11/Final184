#version 450
#include "EngineCommon.h"

#define INOUT in
#include "{{VSInterpolantsHeader}}"
#undef INOUT

layout (location=0) out vec4 GBuffer0;
layout (location=1) out vec4 GBuffer1;

#include "{{MaterialHeader}}"

void main()
{
    GBuffer0 = vec4(Material_GetBaseColor().rgb, 1.0);
    GBuffer1 = vec4(fma(Interpolants_GetWorldNormal(), vec3(0.5), vec3(0.5)), 0.0);
}
