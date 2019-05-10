layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_indirect;
layout(set = 1, binding = 2) uniform texture2D t_depth;

layout(set = 1, binding = 3) uniform EngineCommonMiscs {
    vec2 resolution;
    uint frameCount;
    float frameTime;
};

vec2 invres = 2.0 / resolution;

#define KERNEL_RADIUS 8.0

vec4 SampleColor(vec2 uv)
{
    return vec4(
        texture(sampler2D(t_indirect, s), inUV + uv * invres).rgb,
        texture(sampler2D(t_depth, s), inUV + uv * invres).r
    );
}

float CrossBilateralWeight(float r, float d, float d0) {
	const float BlurSigma = KERNEL_RADIUS * 0.5;
	const float BlurFalloff = 1 / (2 * BlurSigma * BlurSigma);

    float dz = (d0 - d) * 512.0;
	return exp2(-r * r * BlurFalloff - dz * dz);
}

void main(void)
{
	vec4 s = SampleColor(vec2(0.0));
	float center_z = s.w;

	float w = 1.0;
	vec3 total_color = s.rgb * w;
	float total_weight = w;
	float i = 1.0;

	for(; i <= KERNEL_RADIUS/2; i += 1.0)
	{
		s = SampleColor( DIR(i) );
		w = CrossBilateralWeight(i, s.w, center_z);
		total_color += s.rgb * w;
		total_weight += w;

		s = SampleColor( DIR(-i) );
		w = CrossBilateralWeight(i, s.w, center_z);
		total_color += s.rgb * w;
		total_weight += w;
	}

	for(; i <= KERNEL_RADIUS; i += 2.0)
	{
		s = SampleColor( DIR(i + 0.5) );
		w = CrossBilateralWeight(i, s.w, center_z);
		total_color += s.rgb * w;
		total_weight += w;

		s = SampleColor( DIR(-0.5 - i) );
		w = CrossBilateralWeight(i, s.w, center_z);
		total_color += s.rgb * w;
		total_weight += w;
	}

	vec3 color = total_color / total_weight;
	outColor = vec4(color, 0.0);
}