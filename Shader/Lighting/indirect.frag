#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_depth;
layout(set = 1, binding = 2) uniform texture2D t_shadow;
layout(set = 1, binding = 3) uniform texture2D t_normals;
layout(set = 1, binding = 4) uniform texture2D taaBuffer;
layout(rg32ui, set = 1, binding = 5) uniform readonly uimage3D voxels;

layout(set = 1, binding = 6) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
    mat4 VoxelView;
    mat4 VoxelProj;
};

layout(set = 1, binding = 7) uniform Sun {
    vec3 luminance;
    vec3 position; // (or direction, for directional light)
} sun;

layout(set = 1, binding = 8) uniform prevProj {
    mat4 prevProjection;
    mat4 prevModelView;
};

layout(set = 1, binding = 9) uniform EngineCommonMiscs {
    vec2 resolution;
    uint frameCount;
    float frameTime;
};

#include "math.inc"

float getDepth(vec2 uv) {
    return texelFetch(sampler2D(t_depth, s), ivec2(uv * textureSize(t_depth, 0)), 0).r;
}

vec3 getCSpos(vec2 uv) {
    float depth = getDepth(uv);
    vec4 pos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    pos = InvProj * pos;
    return pos.xyz / pos.w;
}

vec3 getNormal(vec2 uv) {
    vec3 raw = fma(texture(sampler2D(t_normals, s), uv).xyz, vec3(2.0), vec3(-1.0));
    return normalize(raw);
}

vec3 lambertBrdf(vec3 wi, vec3 N, vec3 Li, vec3 albedo) {
    return abs(dot(wi, N)) * Li * albedo;
}

float hash(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.2031);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract((p3.x + p3.y) * p3.z);
}

struct HitState {
    vec3 wpos;
    vec3 wnorm;
    vec3 albedo;
    bool hit;
};

vec3 getIndirect(vec3 wpos, vec3 wnorm, inout HitState hit) {
    vec3 Lo = vec3(0.0);

    const float step_size = 0.1;

    vec2 rands = vec2(hash(inUV + float(frameCount % 8)), hash(vec2(-inUV.y, inUV.x) + float(frameCount % 8)));
    
    vec3 direction = hemisphereSample_cos(rands.x, rands.y);
    if (dot(direction, wnorm) < 0.0) direction = -direction;

    mat4 w2voxel = VoxelProj * VoxelView;

    vec3 march_pos = wpos + direction * (1.0 + rands.y) * step_size / dot(direction, wnorm);

    vec3 startingVoxelPos = (w2voxel * vec4(wpos, 1.0)).xyz;
    startingVoxelPos.xy = startingVoxelPos.xy * 0.5 + 0.5;
    startingVoxelPos *= 512.0;

    ivec3 starting_voxel = ivec3(startingVoxelPos);

    for (int i = 0; i < 90; i++) {
        march_pos += direction * step_size;

        vec3 voxelPos = (w2voxel * vec4(march_pos, 1.0)).xyz;
        voxelPos.xy = voxelPos.xy * 0.5 + 0.5;
        voxelPos *= 512.0;

        uvec2 voxelData = imageLoad(voxels, ivec3(voxelPos)).rg;

        if (voxelData.r != 0) {
            vec3 voxColor = pow(unpackUnorm4x8(voxelData.r).rgb, vec3(2.2));
            vec3 voxNorm = unpackSnorm4x8(voxelData.g).rgb;

            // First bounce lighting
            vec4 spos_proj = (ShadowProj * (ShadowView * vec4(march_pos + voxNorm * 0.06, 1.0)));
            spos_proj /= spos_proj.w;
            spos_proj.xy = spos_proj.xy * 0.5 + 0.5;
            float shadowZ = texelFetch(sampler2D(t_shadow, s), ivec2(spos_proj.xy * 2048.0), 0).x;
            float shade = step(spos_proj.z + 0.005, shadowZ);

            Lo += lambertBrdf(-sun.position, voxNorm, sun.luminance * shade, voxColor);

            hit.wpos = march_pos;
            hit.wnorm = voxNorm;
            hit.albedo = voxColor;
            hit.hit = true;

            break;
        }
    }

    return Lo;
}

void main() {
    vec3 cspos = getCSpos(inUV);
    vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;

    vec3 cspos_cam = vec3(0.0);
    vec3 wpos_cam = (InvModelView * vec4(cspos_cam, 1.0)).xyz;

    vec3 csnorm = getNormal(inUV);
    vec3 wnorm = mat3(InvModelView) * csnorm;

    HitState state;
    state.hit = false;

    vec3 indirect = getIndirect(wpos, wnorm, state);

    outColor = vec4(indirect, 1.0);
}
