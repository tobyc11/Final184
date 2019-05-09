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
    CLight(tc::Color c = { 1.0, 1.0, 1.0 }, float i = 1.0, ELightType type = ELightType::Point);

    ELightType getType() const { return Type; }
    tc::Color getLuminance() const { return Color * Intensity; }

private:
    ELightType Type;
    tc::Color Color;
    float Intensity;
};

} /* namespace Foreground */
