#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out VertexData {
    vec2 uv;
    vec3 color;
    vec3 n;
};

layout(location = 0) in vec3 v_vertex;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_color;

layout(binding = 1) uniform UBO {
	vec4 uniformColor;
    mat4 projection;
    mat4 modelview;
    mat4 projectionInverse;
    float near;
    float far;
};

void main() {
    vec4 position = vec4(v_vertex, 1.0);
    position = modelview * position;
    position = projection * position;

    n = mat3(projection) * v_normal;
	uv = v_uv;
    color = v_color;

    gl_Position = position;
}
