#include "Light.h"

namespace Foreground
{

CLight::CLight(tc::Color c, float i, ELightType type)
{
    Color = c;
    Intensity = i;
    Type = type;
};

}
