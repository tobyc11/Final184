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

#include "math.inc"

const int directionSamples = 8;
const int horizonSamples = 8;
const float cutoff = 32.0;

const float attenuation = 1.0;

vec3 multiBounce(float visibility, vec3 albedo) {
    vec3 a =  2.0404 * albedo - 0.3324;
    vec3 b = -4.7951 * albedo + 0.6417;
    vec3 c =  2.7552 * albedo + 0.6903;

    return max(vec3(visibility), ((visibility * a + b) * visibility + c) * visibility);
}

float getVisibility(vec2 uv) {
    vec3 curr_vpos = getCSpos(uv);
    vec3 V = -normalize(curr_vpos);

    if (-curr_vpos.z > cutoff) return 1.0;

    vec3 cnorm = getNormal(uv);

    float integral = 0.0;

    vec2 normalMapDims = textureSize(t_normals, 0).xy;

    float radius = normalMapDims.y * 0.5 / -curr_vpos.z;
    float totalWeight = 0.0;

    float phi = -rand21(uv) * PI / float(directionSamples);
    float rStep = radius / float(horizonSamples / 2);

    for (int samp = 0; samp < directionSamples; samp++) {
        phi += PI / float(directionSamples);

        vec2 h = vec2(-1.0);

        vec3 sliceDir = vec3(spherical(phi), 0.0);
        vec2 sliceDirection = vec2(sliceDir.x, -sliceDir.y);

        float r = rStep * rand21(uv + vec2(samp));

        for (int j = 0; j < horizonSamples / 2; j++) {
            vec2 offset = r * sliceDirection / normalMapDims;
            r += rStep;

            vec2 uv1 = uv - offset; // Tangent direction
            vec2 uv2 = uv + offset; // Opposite to angent direction

            vec3 ds = getCSpos(uv1) - curr_vpos;
            vec3 dt = getCSpos(uv2) - curr_vpos;

            vec2 hs = vec2(dot(V, normalize(ds)), dot(V, normalize(dt)));

            if (clamp(uv1, vec2(0.0), vec2(1.0)) != uv1) hs.x = -1.0;
            if (clamp(uv2, vec2(0.0), vec2(1.0)) != uv2) hs.y = -1.0;

            // Thicknes heuristics
            const float blendFactor = 0.5;
            vec2 flag = step(hs, h);
            h = mix(mix(h, hs, blendFactor), max(h, hs), flag);
        }

        h = fastAcos(h);

        vec3 sliceNormal = normalize(cross(V, sliceDir));
        vec3 sliceBitangent = normalize(cross(sliceNormal, V));

        vec3 projNorm = cnorm - sliceNormal * dot(cnorm, sliceNormal);
        float weight = length(projNorm) + 1e-6;
        projNorm /= weight;

        float cosn = dot(projNorm, V);
        float sinn = dot(projNorm, sliceBitangent);
        float n = fastAcos(cosn) * sign(sinn);

        h.x = n + max(-h.x - n, -half_PI);
        h.y = n + min( h.y - n,  half_PI);

        float a = 0.25 * dot(-cos(2.0 * h - n) + cos(n) + 2.0 * h * sin(n), vec2(1.0));

        integral += a * weight;
    }

    return integral / float(directionSamples);
}

void main() {
    float visibility = getVisibility(inUV);

    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;

    outColor = vec4(vec3(visibility), 1.0);
}