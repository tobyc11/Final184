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