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
layout(set = 1, binding = 4) uniform texture2D temporal;
layout(rg16ui, set = 1, binding = 5) uniform readonly uimage3D voxels;

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

struct HitState {
    vec3 wpos;
    vec3 wnorm;
    vec3 albedo;
    bool hit;
};

mat3 make_coord_space(vec3 n) {
  vec3 z = vec3(n.x, n.y, n.z);
  vec3 h = z;

  if (abs(h.x) <= abs(h.y) && abs(h.x) <= abs(h.z)) h.x = 1.0;
  else if (abs(h.y) <= abs(h.x) && abs(h.y) <= abs(h.z)) h.y = 1.0;
  else h.z = 1.0;

  z = normalize(z);
  vec3 y = cross(h, z);
  y = normalize(y);
  vec3 x = cross(z, y);
  x = normalize(x);

  return mat3(x, y, z);
}

vec3 unpackColor565(uint x) {
    return vec3(
        float((x & 0XF800) >> 11) / 31.0,
        float((x & 0x7E0 ) >>  5) / 63.0,
        float((x & 0x1F  )      ) / 31.0
    );
}

vec3 unpackNormal565(uint x) {
    return normalize(vec3(
        float((x & 0x1F  )      ) / 16.0 - 1.0,
        float((x & 0x7E0 ) >>  5) / 32.0 - 1.0,
        float((x & 0XF800) >> 11) / 16.0 - 1.0
    ));
}

vec3 getIndirect(vec3 wpos, vec3 wnorm, float randSeed, inout HitState hit) {
    hit.hit = false;
    
    vec3 Lo = vec3(0.0);

    const float step_size = 0.2;

    iiTime = float(frameCount) * 0.03125;

    vec2 rands;
    float random_addition1 = hash(vec2(randSeed));
    float random_addition2 = hash(vec2(randSeed));
    rands.x = blugausnoise2(-(inUV + random_addition1) * resolution);
    rands.y = blugausnoise2( (inUV + random_addition2) * resolution);
    
    vec3 direction = hemisphereSample_cos(rands.x, rands.y);
    mat3 o2w = make_coord_space(wnorm);
    direction = o2w * direction;

    mat4 w2voxel = VoxelProj * VoxelView;

    vec3 march_pos = wpos + direction * (1.0 + rands.y) * step_size / dot(direction, wnorm);

    vec3 startingVoxelPos = (w2voxel * vec4(wpos, 1.0)).xyz;
    startingVoxelPos.xy = startingVoxelPos.xy * 0.5 + 0.5;
    startingVoxelPos *= 256.0;

    ivec3 starting_voxel = ivec3(startingVoxelPos);

    for (int i = 0; i < 40; i++) {
        march_pos += direction * step_size;

        vec3 voxelPos = (w2voxel * vec4(march_pos, 1.0)).xyz;
        voxelPos.xy = voxelPos.xy * 0.5 + 0.5;
        voxelPos *= 256.0;

        uvec2 voxelData = imageLoad(voxels, ivec3(voxelPos)).rg;

        if (voxelData.r != 0) {
            vec3 voxColor = pow(unpackColor565(voxelData.r), vec3(2.2));
            vec3 voxNorm = unpackNormal565(voxelData.g).rgb;

            // First bounce lighting
            vec4 spos_proj = (ShadowProj * (ShadowView * vec4(march_pos + voxNorm * 0.06, 1.0)));
            spos_proj /= spos_proj.w;
            spos_proj.xy = spos_proj.xy * 0.5 + 0.5;
            float shadowZ = texelFetch(sampler2D(t_shadow, s), ivec2(spos_proj.xy * 2048.0), 0).x;
            float shade = step(spos_proj.z + 0.005, shadowZ);

            Lo += lambertBrdf(-sun.position, voxNorm, sun.luminance * shade, voxColor) / max(0.01, dot(direction, wnorm));

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

    vec3 indirect;

    indirect  = getIndirect(wpos, wnorm, 0.0, state);
    indirect += getIndirect(wpos, wnorm, 1.0, state);
    indirect += getIndirect(wpos, wnorm, 2.0, state);
    indirect += getIndirect(wpos, wnorm, 3.0, state);
    indirect *= 0.25;

    indirect += vec3(0.01);

    // And then TAA
    vec4 prevCspos = prevModelView * vec4(wpos, 1.0);
    vec4 prevProjPos = prevProjection * prevCspos;
    prevProjPos /= prevProjPos.w;

    prevProjPos.st = prevProjPos.st * 0.5 + 0.5;

    vec2 reprojUV = prevProjPos.st;

    if (clamp(reprojUV, vec2(0.0), vec2(1.0)) == reprojUV) {
        float blendWeight = 0.95;
        vec3 prevColor = texture(sampler2D(temporal, s), reprojUV).rgb;
        indirect = mix(indirect, prevColor, blendWeight);
    }

    outColor = vec4(indirect, 1.0);
}
