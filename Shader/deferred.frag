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

const int samplesCount = 12;
const float PI = 3.1415926f;
const float radius = 0.3;
const float cutoff = 32.0;

const float attenuation = 1.0;

const vec2 poisson_12[12] = vec2 [] (
	vec2(-0.326212, -0.40581),
	vec2(-0.840144, -0.07358),
	vec2(-0.695914,  0.457137),
	vec2(-0.203345,  0.620716),
	vec2(0.96234,   -0.194983),
	vec2(0.473434,  -0.480026),
	vec2(0.519456,   0.767022),
	vec2(0.185461,  -0.893124),
	vec2(0.507431,   0.064425),
	vec2(0.89642,    0.412458),
	vec2(-0.32194,  -0.932615),
	vec2(-0.791559, -0.59771)
);

float getAO(vec2 uv) {
    vec3 cpos = getCSpos(uv);

    if (-cpos.z > cutoff) return 1.0;

    vec3 cnorm = texture(sampler2D(t_normals, s), uv).rgb * 2.0 - 1.0;

    float ao = 0.0;

    float adjRadius = radius / cpos.z;

    float d = rand(uv) * 0.5 + 0.5;
    
    for (int i = 0; i < samplesCount; i++) {

        vec2 offset = poisson_12[i] * d * adjRadius;

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

    outColor = vec4(ao, 1.0);
}
