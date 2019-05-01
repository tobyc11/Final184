#pragma once
#include <Vector4.h>

namespace Foreground
{

// Please extend and specialize this class
class CBasicMaterial
{
public:
    CBasicMaterial() = default;

    void SetAlbedo(tc::Vector4 value) { Albedo = std::move(value); }
    void SetMetallic(float value) { Metallic = value; }
    void SetRoughness(float value) { Roughness = value; }

    const tc::Vector4& GetAlbedo() const { return Albedo; }
    float GetMetallic() const { return Metallic; }
    float GetRoughness() const { return Roughness; }

private:
    tc::Vector4 Albedo;
    float Metallic;
    float Roughness;
};

} /* namespace Foreground */
