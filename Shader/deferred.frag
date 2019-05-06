#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_normals;
layout(set = 1, binding = 3) uniform texture2D t_depth;

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

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

const int directionSamples = 8;
const int horizonSamples = 8;
const float PI = 3.1415926f;
const float cutoff = 32.0;

const float attenuation = 1.0;

vec2 spherical(float phi) {
    return vec2(cos(phi), sin(phi));
}

float getAO(vec2 uv) {
    vec3 curr_vpos = getCSpos(uv);
    vec3 V = -normalize(curr_vpos);

    if (-curr_vpos.z > cutoff) return 1.0;

    vec3 cnorm = getNormal(uv);

    float ao = 0.0;

    vec2 normalMapDims = textureSize(t_normals, 0).xy;

    float radius = normalMapDims.y * 0.5 / -curr_vpos.z;
    float totalWeight = 0.0;

    float phi = -rand(uv) * PI / float(directionSamples);

    for (int samp = 0; samp < directionSamples; samp++) {
        phi += PI / float(directionSamples);

        vec2 h = vec2(-1.0);

        vec3 sliceDir = vec3(spherical(phi), 0.0);
        vec2 sliceDirection = vec2(sliceDir.x, -sliceDir.y);

        for (int j = 0; j < horizonSamples / 2; j++) {
            vec2 offset = radius * (float(j) + rand(uv + vec2(samp, j))) / float(horizonSamples / 2) * sliceDirection / normalMapDims;

            vec2 uv1 = uv - offset; // Tangent direction
            vec2 uv2 = uv + offset; // Opposite to angent direction

            vec3 ds = getCSpos(uv1) - curr_vpos;
            vec3 dt = getCSpos(uv2) - curr_vpos;

            vec2 hs = vec2(dot(V, normalize(ds)), dot(V, normalize(dt)));

            if (clamp(uv1, vec2(0.0), vec2(1.0)) != uv1) hs.x = -1.0;
            if (clamp(uv2, vec2(0.0), vec2(1.0)) != uv2) hs.y = -1.0;

            h = max(h, hs);
        }

        h = acos(h);

        vec3 sliceNormal = normalize(cross(V, sliceDir));
        vec3 sliceBitangent = normalize(cross(sliceNormal, V));

        vec3 projNorm = cnorm - sliceNormal * dot(cnorm, sliceNormal);
        float weight = length(projNorm) + 1e-6;
        projNorm /= weight;

        float cosn = dot(projNorm, V);
        float sinn = dot(projNorm, sliceBitangent);
        float n = acos(cosn) * sign(sinn);

        h.x = n + max(-h.x - n, -PI/2);
        h.y = n + min( h.y - n, PI/2);

        float a = 0.25 * dot(-cos(2.0 * h - n) + cos(n) + 2.0 * h * sin(n), vec2(1.0));

        ao += a * weight;
    }

    return ao / float(directionSamples);
}

void main() {
    vec3 ao = vec3(getAO(inUV));

    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;

    outColor = vec4(ao, 1.0);
}