#include <utility>

#pragma once
#include <Resources.h>
#include <Pipelang.h>
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

    void SetAlbedo(tc::Vector4 value) { Albedo = value; bDSDirty = true; }
    void SetMetallic(float value) { Metallic = value; bDSDirty = true; }
    void SetRoughness(float value) { Roughness = value; bDSDirty = true; }
    void SetAlbedoImage(RHI::CImageView::Ref image) { AlbedoImage = std::move(image); bDSDirty = true; }
    void SetMetallicRoughnessImage(RHI::CImageView::Ref image) { MetallicRoughnessImage = std::move(image); bDSDirty = true; }

    const tc::Vector4& GetAlbedo() const { return Albedo; }
    float GetMetallic() const { return Metallic; }
    float GetRoughness() const { return Roughness; }
    RHI::CImageView::Ref GetAlbedoImage() const { return AlbedoImage; }
    RHI::CImageView::Ref GetMetallicRoughnessImage() const { return MetallicRoughnessImage; }

    void Bind(RHI::IRenderContext& context);

private:
    std::string Name;

    tc::Vector4 Albedo;
    float Metallic{};
    float Roughness{};

    RHI::CImageView::Ref AlbedoImage;
    RHI::CImageView::Ref MetallicRoughnessImage;

    struct MaterialConstants
    {
        tc::Vector4 BaseColor;
        tc::Vector4 MetallicRoughness;
        uint32_t UseTextures;
    };

    bool bDSDirty = true;
    RHI::CDescriptorSet::Ref DescriptorSet;
};

} /* namespace Foreground */
