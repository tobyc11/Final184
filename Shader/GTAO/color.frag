#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_ao;
layout(set = 1, binding = 3) uniform texture2D t_lighting;
layout(set = 1, binding = 4) uniform texture2D t_shadow;

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

void main() {
    // Albedo has a gamma = 2.2
    vec3 albedo = texture(sampler2D(t_albedo, s), inUV).rgb;
    albedo = pow(albedo, vec3(2.2));
    
    vec3 ao = texture(sampler2D(t_ao, s), inUV).rrr;
    vec3 lighting = texture(sampler2D(t_lighting, s), inUV).rgb;
    vec3 color = albedo * (ao * 0.2 + lighting);

    tonemap(color, 1.0);

    outColor = vec4(color, 1.0);
}
