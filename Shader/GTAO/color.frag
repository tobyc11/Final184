#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outTAA;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_ao;
layout(set = 1, binding = 3) uniform texture2D t_depth;
layout(set = 1, binding = 4) uniform texture2D t_lighting;
layout(set = 1, binding = 5) uniform texture2D t_shadow;
layout(set = 1, binding = 6) uniform texture2D t_normals;
layout(set = 1, binding = 7) uniform texture2D taaBuffer;
layout(rg32ui, set = 1, binding = 8) uniform readonly uimage3D voxels;

layout(set = 1, binding = 9) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
    mat4 VoxelView;
    mat4 VoxelProj;
};

layout(set = 1, binding = 10) uniform Sun {
    vec3 luminance;
    vec3 position; // (or direction, for directional light)
} sun;

layout(set = 1, binding = 11) uniform prevProj {
    mat4 prevProjection;
    mat4 prevModelView;
};

layout(set = 1, binding = 12) uniform EngineCommonMiscs {
    vec2 resolution;
    uint frameCount;
    float frameTime;
};

void tonemap(inout vec3 color, float adapted_lum) {
	color *= adapted_lum;

	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	color = (color*(a*color+b))/(color*(c*color+d)+e);

	color = pow(color, vec3(1.0/2.2));
}

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

const float g = .76;
const float g2 = g * g;

const float R0 = 6370e3;
const float Ra = 6425e3;
const float Hr = 10e3;
const float Hm = 2.7e3;

const vec3 I0 = vec3(20.0);
const vec3 bR = vec3(5.8e-6, 13.5e-6, 33.1e-6);

const int steps = 5;
const int stepss = 3;

vec3 I = I0;

const vec3 C = vec3(0., -R0, 0.);
const vec3 bM = vec3(31e-6);

void densities(in vec3 pos, out vec2 des) {
	// des.x = Rayleigh
	// des.y = Mie
	float h = length(pos - C) - R0;
	des.x = exp(-h/Hr);

	des.x += exp(-max(0.0, (h - 35e3)) /  5e3) * exp(-max(0.0, (35e3 - h)) / 15e3) * 0.2;

	des.y = exp(-h/Hm);
}

float escape(in vec3 p, in vec3 d, in float R) {
	vec3 v = p - C;
	float b = dot(v, d);
	float c = dot(v, v) - R*R;
	float det2 = b * b - c;
	if (det2 < 0.) return -1.;
	float det = sqrt(det2);
	float t1 = -b - det, t2 = -b + det;
	return (t1 >= 0.) ? t1 : t2;
}

const float c_precision = 128.0;
const float c_precisionp1 = c_precision + 1.0;

vec3 float2color(float value) {
	vec3 color;
	color.r = mod(value, c_precisionp1) / c_precision;
	color.b = mod(floor(value / c_precisionp1), c_precisionp1) / c_precision;
	color.g = floor(value / (c_precisionp1 * c_precisionp1)) / c_precision;
	return color;
}

// this can be explained: http://www.scratchapixel.com/lessons/3d-advanced-lessons/simulating-the-colors-of-the-sky/atmospheric-scattering/
vec3 scatter(vec3 o, vec3 d, vec3 Ds, float l) {
	if (d.y < 0.0) d.y = 0.0016 / (-d.y + 0.04) - 0.04;

	float L = min(l, escape(o, d, Ra));
	float mu = dot(d, Ds);
	float opmu2 = 1. + mu*mu;
	float phaseR = .0596831 * opmu2;
	float phaseM = .1193662 * (1. - g2) * opmu2;
	phaseM /= ((2. + g2) * pow(1. + g2 - 2.*g*mu, 1.5));

	vec2 depth = vec2(0.0);
	vec3 R = vec3(0.), M = vec3(0.);

	float u0 = - (L - 100.0) / (1.0 - exp2(steps));

	float dither = 0.0;//fma(rand21(d.xy + d.zz), 0.5, 0.5);

	for (int i = 0; i < steps; ++i) {
		float dl = u0 * exp2(i - dither);
		float l = - u0 * (1.0 - exp2(i - dither + 1));
		vec3 p = o + d * l;

		vec2 des;
		densities(p, des);
		des *= vec2(dl);
		depth += des;

		float Ls = escape(p, Ds, Ra);
		if (Ls > 0.) {
			//float dls = Ls;
			vec2 depth_in = vec2(0.0);
			for (int j = 0; j < stepss; ++j) {
				float ls = float(j) / float(stepss) * Ls;
				vec3 ps = p + Ds * ls;
				vec2 des_in;
				densities(ps, des_in);
				depth_in += des_in;
			}
			depth_in *= vec2(Ls) / float(stepss);
			depth_in += depth;

			vec3 A = exp(-(bR * depth_in.x + bM * depth_in.y));

			R += A * des.x;
			M += A * des.y;
		} else {
			return vec3(0.);
		}
	}

	vec3 color = I * (R * bR * phaseR + M * bM * phaseM);
	return max(vec3(0.0), color);
}

const int samplesVL = 16;

float miePhase(vec3 dir) {
	float mu = normalize(dir).z;
	float opmu2 = 1. + mu*mu;
	float phaseR = .0596831 * opmu2;
	float phaseM = .1193662 * (1. - g2) * opmu2;

    return phaseM;
}

void VL(inout vec3 color, vec3 wpos, vec3 wpos_cam) {
    vec3 spos = (ShadowView * vec4(wpos, 1.0)).xyz;
    vec3 spos_cam = (ShadowView * vec4(wpos_cam, 1.0)).xyz;

    float dither = rand21(inUV);

    vec3 sample_delta = (spos - spos_cam) / float(samplesVL);
    vec3 sample_pos = spos_cam + sample_delta * dither;

    float contribute_factor = 0.0;

    for (int i = 0; i < samplesVL; i++) {
        sample_pos += sample_delta;

        vec4 spos_proj = ShadowProj * vec4(sample_pos, 1.0);
        spos_proj /= spos_proj.w;
        spos_proj.xy = spos_proj.xy * 0.5 + 0.5;

        float shadowZ = texelFetch(sampler2D(t_shadow, s), ivec2(spos_proj.xy * 2048.0), 0).x;
        float shade = step(spos_proj.z, shadowZ);

        contribute_factor += shade * miePhase(sample_pos - spos_cam);
    }

    contribute_factor *= 0.4 / float(samplesVL);

    color = color * (1.0 - contribute_factor * 0.5) + sun.luminance * contribute_factor;
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
    // Albedo has a gamma = 2.2
    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;
    albedo = pow(albedo, vec3(2.2));
   
    vec3 cspos = getCSpos(inUV);
    vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;

    vec3 cspos_cam = vec3(0.0);
    vec3 wpos_cam = (InvModelView * vec4(cspos_cam, 1.0)).xyz;

    vec3 csnorm = getNormal(inUV);
    vec3 wnorm = mat3(InvModelView) * csnorm;

    vec3 ao = texture(sampler2D(t_ao, s), inUV).rrr;
    vec3 lighting = texture(sampler2D(t_lighting, s), inUV).rgb;

    HitState state;
    state.hit = false;

    vec3 inDirect = getIndirect(wpos, wnorm, state);
    //if (state.hit) {
    //    inDirect += state.albedo * getIndirect(state.wpos, state.wnorm, state);
    //}

    vec3 color = inDirect;//albedo * (ao * getIndirect(wpos, cspos, getNormal(inUV)) + lighting);

    if (length(wpos) > 256.0) {
        // Sky
        color = scatter(vec3(0.0, 1e3, 0.0), normalize(wpos), -normalize(sun.position), Ra);
    }

    // Volumetric lighting
    // VL(color, wpos, wpos_cam);

    tonemap(color, 1.0);

    // And then TAA
    vec4 prevCspos = prevModelView * vec4(wpos, 1.0);
    vec4 prevProjPos = prevProjection * prevCspos;
    prevProjPos /= prevProjPos.w;

    prevProjPos.st = prevProjPos.st * 0.5 + 0.5;

    vec2 reprojUV = prevProjPos.st;

    if (clamp(reprojUV, vec2(0.0), vec2(1.0)) == reprojUV) {
        float blendWeight = 0.6;
        vec3 prevColor = texture(sampler2D(taaBuffer, s), reprojUV).rgb;
        color = mix(color, prevColor, blendWeight);
    }

    outTAA = vec4(color, 1.0);

    // Debug voxelization view
    if (inUV.x < 0.3333333 && inUV.y < 0.333333) {
        color = vec3(0.0);

        vec2 uv = inUV * 3.0;
        
        vec3 cspos = getCSpos(uv);
        vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;
        
        vec3 spos = (VoxelProj * (VoxelView * vec4(wpos, 1.0))).xyz;
        vec3 spos_cam = (VoxelProj * (VoxelView * vec4(wpos_cam, 1.0))).xyz;

        spos.xy = spos.xy * 0.5 + 0.5;
        spos *= 512.0;

        spos_cam.xy = spos_cam.xy * 0.5 + 0.5;
        spos_cam *= 512.0;

        vec3 march_pos = spos_cam;
        vec3 march_delta = (spos - spos_cam) / 240.0;

        march_pos += march_delta * rand21(inUV);

        for (int i = 0; i < 256; i++) {
            march_pos += march_delta;
            uvec2 voxelData = imageLoad(voxels, ivec3(march_pos)).rg;

            if (voxelData.r != 0) {
                color = unpackUnorm4x8(voxelData.r).rgb;
                break;
            }
        }
    }

    outColor = vec4(color, 1.0);
}
