#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t_albedo;
layout(set = 1, binding = 2) uniform texture2D t_ao;

void main() {
    vec3 color = texture(sampler2D(t_albedo, s), inUV).xyz;
    vec3 ao = texture(sampler2D(t_ao, s), inUV).xxx;
    outColor = vec4(color * ao, 1.0);
}
