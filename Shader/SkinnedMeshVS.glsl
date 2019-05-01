#version 450

#include "EngineCommon.h"

layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;
layout (location=2) in vec4 Tangent;
layout (location=3) in vec2 TexCoord0;
layout (location=6) in uvec4 Joints0;
layout (location=7) in vec4 Weights0;

#define INOUT out
#include "StaticMeshInterpolants.h"
#undef INOUT

layout (set=2, binding=0) uniform ModelConstants
{
    mat4 ModelMat;
};

layout (set=2, binding=1) uniform SkinConstants
{
    mat4 Bones[16]; // Any way to make this variable sized?
};

void main()
{
    mat4 skinnedPos;
    skinnedPos[0] = Bones[Joints0[0]] * vec4(Position, 1);
    skinnedPos[1] = Bones[Joints0[1]] * vec4(Position, 1);
    skinnedPos[2] = Bones[Joints0[2]] * vec4(Position, 1);
    skinnedPos[3] = Bones[Joints0[3]] * vec4(Position, 1);
    vec4 modelPos = transpose(skinnedPos) * Weights0;
    vec4 worldPos = ModelMat * modelPos;
    gl_Position = ProjMat * ViewMat * worldPos;
}
