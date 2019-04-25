#pragma once
#include <Vector4.h>

namespace Foreground
{

//Please extend and specialize this class
class CBasicMaterial
{
public:
private:
    tc::Vector4 Albedo;
    float Metallic;
    float Roughness;
};

} /* namespace Foreground */
