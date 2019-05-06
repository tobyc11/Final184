#version 450 core
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
    return texture(sampler2D(t_depth, s), uv).r;
}

vec3 getNormal(vec2 uv) {
    vec3 raw = texture(sampler2D(t_normals, s), uv).xyz;
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

const int samplesCount = 12;
const float PI = 3.1415926f;
const float radius = 0.3;
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
    int m = 100;

    for (int samp = 0; samp < samplesCount; samp++) {
        float phi = rand(uv + vec2(samp)) * PI;

        float ds_z = -1.0;
        float dt_z = -1.0;

        vec2 sliceDirection = spherical(phi);

        vec3 sliceDir = vec3(sliceDirection, 0.0);
        vec3 sliceBitangent = cross(sliceDir, V);
        vec3 sliceTangent = cross(V, sliceBitangent);

        for (int j = 1; j <= m/2; j++) {
            vec2 offset = sliceDirection * float(j) / normalMapDims;

            vec2 uv1 = uv + offset;
            vec2 uv2 = uv - offset;

            vec3 ds = getCSpos(uv1) - curr_vpos;
            vec3 dt = getCSpos(uv2) - curr_vpos;

            ds_z = max(ds_z, dot(V, normalize(ds)));
            dt_z = max(dt_z, dot(V, normalize(dt)));
        }

        float h1 = -acos(ds_z);
        float h2 = acos(dt_z);

        vec3 projNorm = cnorm - sliceBitangent * dot(cnorm, sliceBitangent);
        float weight = length(projNorm) + 1e-6;
        projNorm /= weight;

        float cosn = dot(projNorm, V);
        float sinn = dot(projNorm, sliceTangent);
        float n = atan(sinn / cosn);

        h1 = n + max(h1 - n, -PI/2);
        h2 = n + min(h2 - n, PI/2);

        float a = 0.25 * (-cos(2.0 * h1 - n) + cosn + 2.0 * h1 * sinn)
                + 0.25 * (-cos(2.0 * h2 - n) + cosn + 2.0 * h2 * sinn);

        ao += a * weight;
    }

    return ao / samplesCount;
}

void main() {
    vec3 ao = vec3(getAO(inUV));

    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;

    outColor = vec4(ao, 1.0);
}
