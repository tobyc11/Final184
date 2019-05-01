#version 450
#include "EngineCommon.h"

#define INOUT in
#include "StaticMeshInterpolants.h"
#undef INOUT

layout (location=0) out vec4 Color0;

void main()
{
    Color0 = vec4((sin(Interpolants_GetWorldPosition()) + 1) / 2, 1);
}
