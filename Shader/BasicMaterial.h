// Another instance of poor man's OOP
//   You can create another material like this with identical functions
//   and it will fit into any surface pixel shader

// Use set 1 for materials?
layout (set=1, binding=0) uniform MaterialConstants
{
    vec4 BaseColor;
    vec4 MetallicRoughness;
    bool UseTextures;
};
//layout (set=1, binding=1) uniform texture2D BaseColorTex;
//layout (set=1, binding=2) uniform texture2D MetallicRoughnessTex;

vec4 Material_GetBaseColor()
{
    if (!UseTextures)
        return BaseColor;
    return BaseColor;
    //vec2 uv = Interpolants_GetTexCoord0();
    //return texture(sampler2D(BaseColorTex, GlobalNiceSampler), uv);
}

float Material_GetMetallic()
{
    return MetallicRoughness.g;
}

float Material_GetRoughness()
{
    return MetallicRoughness.b;
}
