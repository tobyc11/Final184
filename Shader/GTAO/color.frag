#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

#include "EngineCommon.h"

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_ao;
layout(set = 1, binding = 3) uniform texture2D t_depth;
layout(set = 1, binding = 4) uniform texture2D t_lighting;
layout(set = 1, binding = 5) uniform texture2D t_shadow;
layout(rgba8, set = 1, binding = 6) uniform readonly image3D voxels;

layout(set = 1, binding = 7) uniform ExtendedMatrices {
    mat4 InvModelView;
    mat4 ShadowView;
    mat4 ShadowProj;
    mat4 VoxelView;
    mat4 VoxelProj;
};

layout(set = 1, binding = 8) uniform Sun {
    vec3 luminance;
    vec3 position; // (or direction, for directional light)
} sun;


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


void main() {
    // Albedo has a gamma = 2.2
    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;
    albedo = pow(albedo, vec3(2.2));
    
    vec3 ao = texture(sampler2D(t_ao, s), inUV).rrr;
    vec3 lighting = texture(sampler2D(t_lighting, s), inUV).rgb;
    vec3 color = albedo * (ao * 0.2 + lighting);

    vec3 cspos = getCSpos(inUV);
    vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;

    if (length(wpos) > 256.0) {
        // Sky
        color = scatter(vec3(0.0, 1e3, 0.0), normalize(wpos), -normalize(sun.position), Ra);
    }


    if (inUV.x < 0.3333333 && inUV.y < 0.333333) {
        color = vec3(0.0);

        vec2 uv = inUV * 3.0;
        
        vec3 cspos = getCSpos(uv);
        vec3 wpos = (InvModelView * vec4(cspos, 1.0)).xyz;
        vec3 spos = (VoxelProj * (VoxelView * vec4(wpos, 1.0))).xyz;

        vec3 cspos_cam = vec3(0.0);
        vec3 wpos_cam = (InvModelView * vec4(cspos_cam, 1.0)).xyz;
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
            vec4 voxelColor = imageLoad(voxels, ivec3(march_pos));

            if (voxelColor.a > 0.005) {
                color = voxelColor.rgb;
                break;
            }
        }
    }

    tonemap(color, 1.0);

    outColor = vec4(color, 1.0);
}
