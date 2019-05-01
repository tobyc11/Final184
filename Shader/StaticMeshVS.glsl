#version 450

#include "EngineCommon.h"

layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec4 Tangent;
layout (location=3) in vec2 TexCoord0;

#define INOUT out
#include "StaticMeshInterpolants.h"
#undef INOUT

layout (set=2, binding=0) uniform PerPrimitiveConstants
{
    mat4 ModelMat;
};

void main()
{
    vec4 pos = ModelMat * vec4(Position, 1);
    gl_Position = ProjMat * ViewMat * ModelMat * pos;
    iPosition = (pos / pos.w).xyz;
    iTexCoord0 = TexCoord0;
    iNormal = normalize(mat3(ViewMat) * mat3(ModelMat) * Normal);
}
