#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler s;
layout(set = 0, binding = 1) uniform texture2D t_ao;

void main() {
    vec4 s00 = textureGatherOffset(sampler2D(t_ao, s), inUV, ivec2(-1, -1));
    vec4 s01 = textureGatherOffset(sampler2D(t_ao, s), inUV, ivec2(-1,  1));
    vec4 s10 = textureGatherOffset(sampler2D(t_ao, s), inUV, ivec2( 1, -1));
    vec4 s11 = textureGatherOffset(sampler2D(t_ao, s), inUV, ivec2( 1,  1));

    vec4 sum4 = vec4(
        dot(s00, vec4(1.0)),
        dot(s01, vec4(1.0)),
        dot(s10, vec4(1.0)),
        dot(s11, vec4(1.0))
    );

    float average = dot(sum4, vec4(1.0)) / 16.0;

    outColor = vec4(vec3(average), 1.0);
}