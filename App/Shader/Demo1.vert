#version 450

layout(location = 0) out vec2 outUV;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout(set = 1, binding = 0) uniform MoveOffset {
	vec4 moveOffset; 
	vec4 scale; 
};

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] * scale.xy + moveOffset.xy, 0.0, 1.0);
	outUV = positions[gl_VertexIndex] + vec2(0.5);
}
