const float PI = 3.1415926f;
const float half_PI = PI / 2.0f;
const float PI2 = PI * 2.0;

vec2 spherical(float phi) {
    return vec2(cos(phi), sin(phi));
}

// Pseudo-random generation, vec2 -> float
float rand21(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float fastSqrt(float x) {
    return intBitsToFloat(0x1FBD1DF5 + (floatBitsToInt(x) >> 1));
}

vec2 fastSqrt(vec2 x) {
    return intBitsToFloat(0x1FBD1DF5 + (floatBitsToInt(x) >> 1));
}

float fastAcos(float x) {
    float res = -0.156583 * abs(x) + half_PI;
    res *= fastSqrt(1.0 - abs(x));
    return x >= 0 ? res : PI - res;
}

vec2 fastAcos(vec2 x) {
    vec2 res = -0.156583 * abs(x) + half_PI;
    res *= fastSqrt(1.0 - abs(x));
    vec2 flag = step(x, vec2(0.0));
    return res * fma(-flag, vec2(2.0), vec2(1.0)) + flag * vec2(PI);
}

float w0(float a) {
    return (1.0/6.0)*(a*(a*(-a + 3.0) - 3.0) + 1.0);
}

float w1(float a) {
    return (1.0/6.0)*(a*a*(3.0*a - 6.0) + 4.0);
}

float w2(float a) {
    return (1.0/6.0)*(a*(a*(-3.0*a + 3.0) + 3.0) + 1.0);
}

float w3(float a) {
    return (1.0/6.0)*(a*a*a);
}

// g0 and g1 are the two amplitude functions
float g0(float a) {
    return w0(a) + w1(a);
}

float g1(float a) {
    return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a) {
    return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a) {
    return 1.0 + w3(a) / (w2(a) + w3(a));
}

vec3 hemisphereSample_uniform(float u, float v) {
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
    
vec3 hemisphereSample_cos(float u, float v) {
    float phi = v * 2.0 * PI;
    float cosTheta = sqrt(1.0 - u);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}