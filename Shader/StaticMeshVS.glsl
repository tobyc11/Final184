#version 450

#include "EngineCommon.h"

layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec4 Tangent;
layout (location=3) in vec2 TexCoord0;

#define INOUT out
#include "StaticMeshInterpolants.h"
#undef INOUT

void main()
{
    gl_Position = ProjMat * ViewMat * vec4(Position, 1);
}
