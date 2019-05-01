// This is poor man's OOP
// Those interpolants can be seen as a "polymorphic class"
// Different VS/HS/DS/GS can change those interpolants
// As long as those functions remain, the PS can still call those same functions

layout (location=0) INOUT vec3 iPosition;
layout (location=1) INOUT vec3 iNormal;
layout (location=2) INOUT vec4 iTangent;
layout (location=3) INOUT vec2 iTexCoord0;

vec3 Interpolants_GetWorldPosition()
{
    return iPosition;
}

vec3 Interpolants_GetWorldNormal()
{
    return iNormal;
}

vec2 Interpolants_GetTexCoord0()
{
    return iTexCoord0;
}
