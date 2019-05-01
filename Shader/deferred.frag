#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_normals;
layout(set = 1, binding = 3) uniform texture2D t_depth;

float getDepth(vec2 uv) {
    return texture(sampler2D(t_depth, s), uv).r;
}

vec3 getCSpos(vec2 uv) {
    float depth = getDepth(uv);
    vec4 pos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    pos = InvProj * pos;
    return pos.xyz / pos.w;
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

const int samplesCount = 24;
const float PI = 3.1415926f;
const float radius = 1.0;
const float cutoff = 32.0;

const float attenuation = 1.0;

float getAO(vec2 uv) {
    vec3 cpos = getCSpos(uv);

    if (-cpos.z > cutoff) return 1.0;

    vec3 cnorm = texture(sampler2D(t_normals, s), uv).rgb * 2.0 - 1.0;

    float ao = 0.0;

    float adjRadius = radius / cpos.z;

    for (int i = 0; i < samplesCount; i++) {
        float r = rand(uv + vec2(float(i), 0.0f)) * 2.0f * PI;
        float d = rand(uv + vec2(0.0f, float(i)));

        vec2 offset = vec2(cos(r), sin(r)) * d * adjRadius;

        vec3 spos = getCSpos(uv + offset);

        vec3 diff = spos - cpos;
        float dist = length(diff);

        if (dist > 0.0001) {
            float occlution = dot(cnorm, diff) / dist;

            ao += max(0.0, attenuation - dist / attenuation) * occlution;
        }
    }

    return clamp(pow(1.0 - ao / float(samplesCount), 2.0), 0.0, 1.0);
}

void main() {
    vec3 ao = vec3(0.0);

    ao = vec3(getAO(inUV));

    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;

    outColor = vec4(albedo, 1.0);
}
