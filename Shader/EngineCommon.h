// This descriptor set is bound only a few times per frame
//   Global data and per view data probably fit here

layout (set=0, binding=0) uniform sampler GlobalNiceSampler;
layout (set=0, binding=1) uniform sampler GlobalLinearSampler;
layout (set=0, binding=2) uniform sampler GlobalNearestSampler;
layout (set=0, binding=3) uniform GlobalConstants
{
    vec4 CameraPos;
    mat4 ViewMat;
    mat4 ProjMat;
    mat4 InvProj;
};
