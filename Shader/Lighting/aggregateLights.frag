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

layout(set = 1, binding = 5) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
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

	vec2 p0 = (vec2(iuv.x + h0x, iuv.y + h0y) + 0.5) * shadowPixSize;
	vec2 p1 = (vec2(iuv.x + h1x, iuv.y + h0y) + 0.5) * shadowPixSize;
	vec2 p2 = (vec2(iuv.x + h0x, iuv.y + h1y) + 0.5) * shadowPixSize;
	vec2 p3 = (vec2(iuv.x + h1x, iuv.y + h1y) + 0.5) * shadowPixSize;

	depth = 0.0;
	float texel = texture(sampler2D(t_shadow, s), p0).x; depth += texel;
	float res0 = texel + bias > spos.z ? 1.0 : 0.0;

	texel = texture(sampler2D(t_shadow, s), p1).x; depth += texel;
	float res1 = texel + bias > spos.z ? 1.0 : 0.0;

	texel = texture(sampler2D(t_shadow, s), p2).x; depth += texel;
	float res2 = texel + bias > spos.z ? 1.0 : 0.0;

	texel = texture(sampler2D(t_shadow, s), p3).x; depth += texel;
	float res3 = texel + bias > spos.z ? 1.0 : 0.0;
	depth *= 0.25;

    return g0(fuv.y) * (g0x * res0  +
                        g1x * res1) +
           g1(fuv.y) * (g0x * res2  +
                        g1x * res3);
}


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

        vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;
        vec4 spos = (ShadowProj * (ShadowView * vec4(wpos, 1.0)));

        spos.xy = spos.xy * 0.5 + 0.5;

        float shadowZ;
        float shade = shadowTexSmooth(spos.xyz, shadowZ, 0.001);
       
        result.rgb += L.luminance * cosineLambert * shade;
    }

    lightingBuffer = result;
}