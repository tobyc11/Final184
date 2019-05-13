struct BRDFContext
{
    float NoL;
    float NoV;
    float NoH;
    float VoH;

    vec3 Cdiff;
    vec3 F0;
    float a2;
};

float D_GGX(float a2, float NoH)
{
    float tmp = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * tmp * tmp);
}

vec3 F_Schlick(vec3 F0, float VoH)
{
    return F0 + (vec3(1) - F0) * pow(1.0 - VoH, 5);
}

float Vis_SmithJoint(float a2, float NoV, float NoL)
{
    float smithV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    float smithL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    return 0.5 / (smithV + smithL);
}
