#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_KHR_vulkan_glsl : enable

#include "EngineCommon.h"

const int MAX_LIGHT_COUNT = 100;

struct Light {
    vec3 luminance;
    vec3 position;
};

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 lightingBuffer;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_normals;
layout(set = 1, binding = 3) uniform texture2D t_depth;

layout(set = 2, binding = 0) uniform Lights {
    Light lights[MAX_LIGHT_COUNT];
    int numLights;
};

float getDepth(vec2 uv) {
    return texelFetch(sampler2D(t_depth, s), ivec2(uv * textureSize(t_depth, 0)), 0).r;
}

vec3 getNormal(vec2 uv) {
    vec3 raw = fma(texture(sampler2D(t_normals, s), uv).xyz, vec3(2.0), vec3(-1.0));
    return normalize(raw);
}

vec3 getCSpos(vec2 uv) {
    float depth = getDepth(uv);
    vec4 pos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    pos = InvProj * pos;
    return pos.xyz / pos.w;
}

#include "math.inc"

void main() {
    vec4 result = vec4(0.0, 0.0, 0.0, 1.0);

    vec3 cspos = getCSpos(inUV);

    for (int i = 0; i < numLights; i++) {
        Light L = lights[i];

        vec3 diff = (ViewMat * vec4(L.position, 1.0)).xyz - cspos;
        result.rgb += L.luminance / (1.0 + dot(diff, diff));
    }

    // Add the ambient
    result.rgb += vec3(0.3);

    lightingBuffer = result;
}