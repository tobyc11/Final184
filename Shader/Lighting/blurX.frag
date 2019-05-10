#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : enable

#define DIR(x) vec2(0.0, x)

#include "bilateralBlur.inc"