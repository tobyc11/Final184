#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_depth;
layout(set = 1, binding = 2) uniform texture2D t_shadow;
layout(set = 1, binding = 3) uniform texture2D t_albedo;
layout(set = 1, binding = 4) uniform texture2D t_normals;
layout(set = 1, binding = 5) uniform texture2D temporal;
layout(set = 1, binding = 6) uniform texture3D voxels;
layout(set = 1, binding = 7) uniform texture2D t_material;

layout(set = 1, binding = 8) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
    mat4 VoxelView;
    mat4 VoxelProj;
};

layout(set = 1, binding = 9) uniform Sun {
    vec3 luminance;
    vec3 position; // (or direction, for directional light)
} sun;

layout(set = 1, binding = 10) uniform prevProj {
    mat4 prevProjection;
    mat4 prevModelView;
};

layout(set = 1, binding = 11) uniform EngineCommonMiscs {
    vec2 resolution;
    uint frameCount;
    float frameTime;
};

#include "math.inc"
#include "BRDF.inc"

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

vec2 getMaterial(vec2 uv) {
    return texture(sampler2D(t_material, s), uv).yz;
}

vec3 getBaseColor(vec2 uv) {
    return pow(texture(sampler2D(t_albedo, s), uv).xyz, vec3(2.2));
}

float getMetallic(vec2 uv) {
    return getMaterial(uv).y;
}

float getRoughness(vec2 uv) {
    return getMaterial(uv).x;
}

vec3 lambertBrdf(vec3 wi, vec3 N, vec3 albedo) {
    return abs(dot(wi, N)) * albedo;
}

struct HitState {
    vec3 wpos;
    vec3 wnorm;
    vec3 brdf;
    bool hit;
};

mat3 make_coord_space(vec3 n) {
  vec3 z = normalize(n);
  vec3 x = perp_hm(z);
  vec3 y = cross(z, x);

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
        float((x & 0XF800) >> 11) / 16.0 - 1.0,
        float((x & 0x7E0 ) >>  5) / 32.0 - 1.0,
        float((x & 0x1F  )      ) / 16.0 - 1.0
    ));
}

void BRDF_Init(inout BRDFContext context)
{
    vec3 baseColor = getBaseColor(inUV);
    float metallic = getMetallic(inUV);
    float roughness = getRoughness(inUV);
    float a = roughness * roughness;

    const vec3 dielectricSpecular = vec3(0.04);
    context.Cdiff = mix(baseColor, vec3(0), metallic);
    context.F0 = mix(dielectricSpecular, baseColor, metallic);
    context.a2 = a * a;
}

vec3 BRDF_CookTorrance(BRDFContext context)
{
    // BRDF calculation
    vec3 F = F_Schlick(context.F0, context.VoH);
    float D = D_GGX(context.a2, context.NoH);
    float Vis = Vis_SmithJoint(context.a2, context.NoV, context.NoL);
    vec3 BRDF_Spec = D * F * Vis;

    vec3 BRDF_Diff = (1 - F) * context.Cdiff / PI;

    return BRDF_Diff + BRDF_Spec;
}

vec3 F_Lambert(vec3 Cdiff)
{
    return Cdiff / PI;
}

// vec3 EstimateOneBounceRadiance(BRDFContext context, vec3 wpos, vec3 N, vec3 V, float randSeed)
// {
//     const float step_size = 0.2;

//     iiTime = float(frameCount) * 0.03125;

//     vec2 rands;
//     float random_addition1 = hash(vec2(randSeed));
//     float random_addition2 = hash(vec2(randSeed));
//     rands.x = blugausnoise2(-(inUV + random_addition1) * resolution);
//     rands.y = blugausnoise2( (inUV + random_addition2) * resolution);

//     vec3 L = hemisphereSample_cos(rands.x, rands.y);
//     mat3 l2w = make_coord_space(N);
//     L = normalize(l2w * L);
//     float NoL = dot(N, L);
    
//     mat4 w2voxel = VoxelProj * VoxelView;

//     vec3 march_pos = wpos + L * (1.0 + rands.y) * step_size / NoL;

//     int i = 0;
//     for (; i < 60; i++) {
//         march_pos += L * step_size;

//         vec3 voxelPos = (w2voxel * vec4(march_pos, 1.0)).xyz;
//         voxelPos.xy = voxelPos.xy * 0.5 + 0.5;
//         //voxelPos *= 256.0;

//         vec2 voxelData = textureLod(sampler3D(voxels, s), voxelPos, 0).rg;
//         //uvec2 voxelData = imageLoad(voxels, ivec3(voxelPos)).rg;

//         if (voxelData.r != 0) {
//             vec3 voxColor = pow(unpackColor565(voxelData.r), vec3(2.2));
//             vec3 voxNorm = unpackNormal565(voxelData.g).rgb;

//             // First bounce lighting
//             vec4 spos_proj = (ShadowProj * (ShadowView * vec4(march_pos + voxNorm * 0.06, 1.0)));
//             spos_proj /= spos_proj.w;
//             spos_proj.xy = spos_proj.xy * 0.5 + 0.5;
//             float shadowZ = texelFetch(sampler2D(t_shadow, s), ivec2(spos_proj.xy * 2048.0), 0).x;
//             float shade = step(spos_proj.z + 0.005, shadowZ);

//             vec3 H = normalize(V + L);
//             context.NoL = NoL;
//             context.NoV = dot(N, V);
//             context.NoH = dot(N, H);
//             context.VoH = dot(V, H);

//             // Fr(Wi, Wo) * cos(Theta) / PDF = Fr(Wi, Wo) * PI
//             vec3 weight = BRDF_CookTorrance(context) * PI;

//             vec3 Esun = sun.luminance * dot(-sun.position, voxNorm) * shade;
//             vec3 Lvox_direct = F_Lambert(voxColor) * Esun;
//             return Lvox_direct * weight;
//         }
//     }

//     return vec3(0.7, 0.8, 1.0) * 0.4 / max(0.01, NoL) * smoothstep(0.0, 0.01, NoL);
// }

const bool second_bounce = true;

const float VOXEL_SIZE = 0.078125;

vec4 VCTConeTrace(vec3 pos, vec3 N, vec3 dir, float coneApeature)
{
    vec3 c = vec3(0);
    float a = 0.0;

    float t = VOXEL_SIZE;
    vec3 startPos = pos + N * VOXEL_SIZE * sqrt(3) * 3;

    while (t < 25.0 && a < 1.0)
    {
        float diameter = max(VOXEL_SIZE, 2 * coneApeature * t);
        float mip = log2(diameter / VOXEL_SIZE);

        vec3 worldPos = startPos + t * dir;
        vec4 voxelNDC = VoxelProj * VoxelView * vec4(worldPos, 1.0);
        voxelNDC /= voxelNDC.w;
        voxelNDC.xy = voxelNDC.xy * 0.5 + 0.5;
        vec4 voxelData = textureLod(sampler3D(voxels, s), voxelNDC.xyz, mip);
        float a2 = voxelData.a;
        vec3 c2 = voxelData.rgb;
        c += (1 - a) * a2 * c2;
        a += (1 - a) * a2;

        t += diameter * VOXEL_SIZE * 2.0;
    }
    return vec4(c, t);
}

vec4 VCTConeTraceSpecular(vec3 pos, vec3 N, vec3 dir, float coneApeature)
{
    vec3 c = vec3(0);
    float a = 0.0;

    float t = VOXEL_SIZE;
    vec3 startPos = pos + N * VOXEL_SIZE * sqrt(3) * 3;

    while (t < 25.0 && a < 1.0)
    {
        float diameter = max(VOXEL_SIZE, 2 * coneApeature * t);
        float mip = log2(diameter / VOXEL_SIZE);

        vec3 worldPos = startPos + t * dir;
        vec4 voxelNDC = VoxelProj * VoxelView * vec4(worldPos, 1.0);
        voxelNDC /= voxelNDC.w;
        voxelNDC.xy = voxelNDC.xy * 0.5 + 0.5;
        vec4 voxelData = textureLod(sampler3D(voxels, s), voxelNDC.xyz, mip);
        float a2 = voxelData.a;
        vec3 c2 = voxelData.rgb;
        c += (1 - a) * a2 * c2;
        a += (1 - a) * a2;

        t += diameter * VOXEL_SIZE * 2.0;
    }
    return vec4(c, t);
}

const vec3 CONES[6] = vec3[6]
(
    vec3(0, 0, 1),
    vec3(0.0, 0.866025, 0.5),
    vec3(0.823639, 0.267617, 0.5),
    vec3(0.509037, -0.700629, 0.5),
    vec3(-0.509037, -0.700629, 0.5),
    vec3(-0.823639, 0.267617, 0.5)
);

const float WEIGHTS[6] = float[6]
(
    PI / 4,
    3 * PI / 20,
    3 * PI / 20,
    3 * PI / 20,
    3 * PI / 20,
    3 * PI / 20
);

vec3 VCTDiffuse(vec3 wpos, vec3 N)
{
    mat3 l2w = make_coord_space(N);
    vec3 radiance = vec3(0);
    for (int i = 0; i < 6; i++)
    {
        vec3 coneDir = CONES[i];
        coneDir = normalize(coneDir);
        if (dot(coneDir, N) < 0)
            coneDir = -coneDir;

        radiance += VCTConeTrace(wpos, N, coneDir, tan(PI * 0.5 * 0.66)).rgb * WEIGHTS[i];
    }

    return max(vec3(0), radiance) * getBaseColor(inUV);
}

void main() {
    BRDFContext context;
    BRDF_Init(context);

    vec3 cspos = getCSpos(inUV);
    vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;

    vec3 cspos_cam = vec3(0.0);
    vec3 wpos_cam = (InvModelView * vec4(cspos_cam, 1.0)).xyz;

    vec3 csnorm = getNormal(inUV);
    vec3 wnorm = mat3(InvModelView) * csnorm;

    vec3 N = normalize(wnorm);
    vec3 V = normalize(wpos_cam - wpos);

    vec3 indirect = vec3(0);

    indirect += VCTDiffuse(wpos, N) * 10.0;

    // indirect += EstimateOneBounceRadiance(context, wpos, N, V, 0.0);
    // indirect += EstimateOneBounceRadiance(context, wpos, N, V, 2.0);
    // indirect += EstimateOneBounceRadiance(context, wpos, N, V, 5.0);
    // indirect += EstimateOneBounceRadiance(context, wpos, N, V, 9.0);
    // indirect /= 4.0;

    vec3 L = reflect(-V, N);

    float NoV = abs(dot(N, V));
    float NoL = abs(dot(N, L));

    mat4 world2Voxel = VoxelProj * VoxelView;

    vec3 c = vec3(0);
    float a = 0.0;

    float t = VOXEL_SIZE; // 0.4 gets rid of most self intersection
    vec3 p0 = wpos + N * VOXEL_SIZE * sqrt(3) * 3;
    vec3 d = normalize(L);
    while (t < 25.0 && a < 1.0)
    {
        float diameter = max(VOXEL_SIZE, 2 * tan(PI * 0.5 * 0.1) * t);
        float mip = log2(diameter / VOXEL_SIZE);

        vec3 worldPos = p0 + t * d;
        vec4 voxelNDC = VoxelProj * VoxelView * vec4(worldPos, 1.0);
        voxelNDC /= voxelNDC.w;
        voxelNDC.xy = voxelNDC.xy * 0.5 + 0.5;
        vec4 voxelData = textureLod(sampler3D(voxels, s), voxelNDC.xyz, mip);
        float a2 = voxelData.a;
        vec3 c2 = voxelData.rgb;
        c += (1 - a) * a2 * c2;
        a += (1 - a) * a2;

        t += diameter * VOXEL_SIZE * 2.0;
    }
    indirect += c * context.F0;
    indirect += context.Cdiff * vec3(0.01);

    // HitState state;

    // vec3 indirect;

    // indirect  = getIndirect(wpos, wnorm, 0.0, state);
    // if (state.hit && second_bounce) {
    //     indirect += state.brdf * getIndirect(state.wpos, state.wnorm, 1.0, state);
    // }

    // indirect += getIndirect(wpos, wnorm, 2.0, state);
    // if (state.hit && second_bounce) {
    //     indirect += state.brdf * getIndirect(state.wpos, state.wnorm, 3.0, state);
    // }

    // indirect += getIndirect(wpos, wnorm, 4.0, state);
    // if (state.hit && second_bounce) {
    //     indirect += state.brdf * getIndirect(state.wpos, state.wnorm, 5.0, state);
    // }

    // indirect += getIndirect(wpos, wnorm, 6.0, state);
    // if (state.hit && second_bounce) {
    //     indirect += state.brdf * getIndirect(state.wpos, state.wnorm, 7.0, state);
    // }

    // indirect *= 0.25;

    // And then TAA
    // vec4 prevCspos = prevModelView * vec4(wpos, 1.0);
    // vec4 prevProjPos = prevProjection * prevCspos;
    // prevProjPos /= prevProjPos.w;

    // prevProjPos.st = prevProjPos.st * 0.5 + 0.5;

    // vec2 reprojUV = prevProjPos.st;

    // if (clamp(reprojUV, vec2(0.0), vec2(1.0)) == reprojUV) {
    //     float blendWeight = 0.95;
    //     vec4 prevColor = texture(sampler2D(temporal, s), reprojUV);
    //     float prevDepth = prevColor.w;
    //     blendWeight *= smoothstep(0.0, 1.0, 1.0 - abs(prevDepth + cspos.z));

    //     indirect = clamp(mix(indirect, prevColor.rgb, blendWeight), vec3(0.0), vec3(16.0));
    // }

    outColor = vec4(indirect, -cspos.z);
}
