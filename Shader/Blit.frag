#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler s;
layout(set = 1, binding = 1) uniform texture2D t;
layout(set = 1, binding = 2) uniform texture2D td;

void main()
{
	if (inUV.x < 0.5)
	{
		vec2 uv = vec2(inUV.x * 2, inUV.y);
		outColor = texture(sampler2D(td, s), uv).rrrr;
	}
	else
	{
		vec2 uv = vec2((inUV.x - 0.5) * 2, inUV.y);
		outColor = texture(sampler2D(t, s), uv);
	}
}
