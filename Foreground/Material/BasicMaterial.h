#pragma once
#include <Resources.h>
#include <Vector4.h>
#include <string>

namespace Foreground
{

// Please extend and specialize this class
class CBasicMaterial
{
public:
    CBasicMaterial() = default;

    void SetName(std::string name) { Name = std::move(name); }
    const std::string& GetName() const { return Name; }

    void SetAlbedo(tc::Vector4 value) { Albedo = std::move(value); }
    void SetMetallic(float value) { Metallic = value; }
    void SetRoughness(float value) { Roughness = value; }
    void SetAlbedoImage(RHI::CImageView::Ref image) { AlbedoImage = image; }
    void SetMetallicRoughnessImage(RHI::CImageView::Ref image) { MetallicRoughnessImage = image; }

    const tc::Vector4& GetAlbedo() const { return Albedo; }
    float GetMetallic() const { return Metallic; }
    float GetRoughness() const { return Roughness; }
    RHI::CImageView::Ref GetAlbedoImage() const { return AlbedoImage; }
    RHI::CImageView::Ref GetMetallicRoughnessImage() const { return MetallicRoughnessImage; }

private:
    std::string Name;

    tc::Vector4 Albedo;
    float Metallic;
    float Roughness;

    RHI::CImageView::Ref AlbedoImage;
    RHI::CImageView::Ref MetallicRoughnessImage;
};

} /* namespace Foreground */
