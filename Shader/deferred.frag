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

vec2 rotate2d(vec2 v, float a) {
    float s = sin(a);
    float c = cos(a);
    mat2 m = mat2(c, -s, s, c);
    return m * v;
}

float getAO(vec2 uv) {
//    vec3 cpos = getCSpos(uv);
//
//    if (-cpos.z > cutoff) return 1.0;
//
    vec3 cnorm = getNormal(uv);
//
    float ao = 0.0;
//
//    float adjRadius = radius / cpos.z;
//
//    float d = rand(uv) * 0.5 + 0.5;

    vec2 normalMapDims = textureSize(t_normals, 0).xy;
    int m = 100;

    float curr_depth = getDepth(uv);

    for (int samp = 0; samp < samplesCount; samp++) {
        float phi = rand(uv + vec2(samp)) * PI;

        float ds_z = 0;
        float dt_z = 0;

        for (int j = 1; j <= m/2; j++) {
            vec2 dt_xy = rotate2d(vec2(j, 0), phi) / normalMapDims;
            vec2 ds_xy = -dt_xy;

            vec3 ds = vec3(ds_xy,
                           getDepth( uv + ds_xy) - curr_depth);
            vec3 dt = vec3(dt_xy,
                           getDepth( uv + dt_xy) - curr_depth);

            ds_z = max(ds_z, normalize(ds).z);
            dt_z = max(dt_z, normalize(dt).z);
        }

        float h1 = -acos(ds_z);
        float h2 = acos(dt_z);
        float n = acos(cnorm.z);

        h1 = n + max(h1 - n, -PI/2);
        h2 = n + min(h2 - n, PI/2);

        float a = 0.25 * (-cos(2 * h1 - n) + cos(n) + 2 * h1 * sin(n))
                + 0.25 * (-cos(2 * h2 - n) + cos(n) + 2 * h2 * sin(n));

        ao += a * cnorm.z;
    }

    return ao / samplesCount;
}

void main() {
    vec3 ao = vec3(getAO(inUV));

    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;

    outColor = vec4(ao, 1.0);
}
