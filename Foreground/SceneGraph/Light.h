#pragma once
#include <Color.h>

namespace Foreground
{

enum class ELightType
{
    Point,
    Directional,
    SpotLight
};

class CLight
{
public:

private:
    ELightType Type;
    tc::Color Color;
    float Intensity;
};

} /* namespace Foreground */
