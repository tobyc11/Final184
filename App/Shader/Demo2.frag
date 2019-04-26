#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform UBOColor {
	vec4 uniformColor; 
};

layout(set = 0, binding = 0) uniform sampler s;
layout(set = 0, binding = 1) uniform texture2D t;

void main() {
	vec4 sampledColor = texture(sampler2D(t, s), inUV);
    outColor = uniformColor * sampledColor;
}
