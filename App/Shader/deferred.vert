#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out VertexData {
    vec2 uv;
};

const vec2 blit_uv[] = vec2[](
    vec2(-1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0)
);

void main() {
    gl_Position = vec4(blit_uv[gl_VertexIndex], 0.0, 1.0);
    uv = blit_uv[gl_VertexIndex] * 0.5 + 0.5;
}
