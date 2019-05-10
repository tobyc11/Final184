#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_KHR_vulkan_glsl : enable

#include "EngineCommon.h"

const int MAX_LIGHT_COUNT = 100;

struct Light {
    vec3 luminance;
    vec3 position; // (or direction, for directional light)
};

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 lightingBuffer;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_normals;
layout(set = 1, binding = 3) uniform texture2D t_depth;
layout(set = 1, binding = 4) uniform texture2D t_shadow;

layout(set = 1, binding = 7) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
    mat4 VoxelView;
    mat4 VoxelProj;
};

layout(set = 2, binding = 0) uniform pointLights {
    Light lights[MAX_LIGHT_COUNT];
    int numLights;
} Point;

layout(set = 2, binding = 1) uniform directionalLights {
    Light lights[MAX_LIGHT_COUNT];
    int numLights;
} Directional;

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


const float shadowMapResolution = 2048.0;
const vec2 shadowPixSize = vec2(1.0 / shadowMapResolution);

float shadowStep(float d, float z) {
    return step(d, z);
}

float shadowTexSmooth(in vec3 spos, out float depth, in float bias) {
	vec2 uv = spos.xy * vec2(shadowMapResolution) - 1.0;
	vec2 iuv = floor(uv);
	vec2 fuv = uv - iuv;

    float g0x = g0(fuv.x);
    float g1x = g1(fuv.x);
    float h0x = h0(fuv.x) * 0.75;
    float h1x = h1(fuv.x) * 0.75;
    float h0y = h0(fuv.y) * 0.75;
    float h1y = h1(fuv.y) * 0.75;

	ivec2 p0 = ivec2(vec2(iuv.x + h0x, iuv.y + h0y) + 0.5);
	ivec2 p1 = ivec2(vec2(iuv.x + h1x, iuv.y + h0y) + 0.5);
	ivec2 p2 = ivec2(vec2(iuv.x + h0x, iuv.y + h1y) + 0.5);
	ivec2 p3 = ivec2(vec2(iuv.x + h1x, iuv.y + h1y) + 0.5);

	depth = 0.0;
	float texel = texelFetch(sampler2D(t_shadow, s), p0, 0).x; depth += texel;
	float res0 = shadowStep(spos.z, texel + bias);

	texel = texelFetch(sampler2D(t_shadow, s), p1, 0).x; depth += texel;
	float res1 = shadowStep(spos.z, texel + bias);

	texel = texelFetch(sampler2D(t_shadow, s), p2, 0).x; depth += texel;
	float res2 = shadowStep(spos.z, texel + bias);

	texel = texelFetch(sampler2D(t_shadow, s), p3, 0).x; depth += texel;
	float res3 = shadowStep(spos.z, texel + bias);
	depth *= 0.25;

    return g0(fuv.y) * (g0x * res0  +
                        g1x * res1) +
           g1(fuv.y) * (g0x * res2  +
                        g1x * res3);
}

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

void main() {
    vec4 result = vec4(0.0, 0.0, 0.0, 1.0);

    vec3 cspos = getCSpos(inUV);
    vec3 csnorm = getNormal(inUV);

    for (int i = 0; i < Point.numLights; i++) {
        Light L = Point.lights[i];

        vec3 lightVector = (ViewMat * vec4(L.position, 1.0)).xyz - cspos;
        float distSq = dot(lightVector, lightVector);
        float cosineLambert = max(0.0, dot(lightVector / fastSqrt(distSq), csnorm));
        result.rgb += cosineLambert * L.luminance / (1.0 + distSq);
    }

    for (int i = 0; i < Directional.numLights; i++) {
        Light L = Directional.lights[i];

        vec3 lightVector = -normalize(mat3(ViewMat) * L.position);
        float cosineLambert = max(0.0, dot(lightVector, csnorm));

        vec3 wpos = (InvModelView * vec4(cspos + csnorm * 0.01, 1.0)).xyz;
        vec4 spos = (ShadowProj * (ShadowView * vec4(wpos, 1.0)));

        spos.xy = spos.xy * 0.5 + 0.5;

        float shadowZ;

        float shade = 0.0;
        for (int j = 0; j < 12; j++) {
            shade += shadowTexSmooth(spos.xyz + vec3(poisson_12[j] * shadowPixSize, 0.0), shadowZ, 0.002);
        }
        shade /= 12.0;
       
        result.rgb += L.luminance * cosineLambert * shade;
    }

    lightingBuffer = result;
}