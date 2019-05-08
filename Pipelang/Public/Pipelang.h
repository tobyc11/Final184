#pragma once
#include <Device.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Pl
{

class CVertexAttribs
{
public:
    enum class ESemantic : uint32_t
    {
        Position = 1,
        Normal,
        Tangent,
        TexCoord0,
        TexCoord1,
        Color0,
        Joints0,
        Weights0
    };

    const std::map<uint32_t, ESemantic>& GetAttributesByLocation() const;

    // Called from Lua
    void AddAttribute(const std::string& name, uint32_t location);

private:
    std::map<uint32_t, ESemantic> AttribsByLocation;
};

class CParameterBlock;

// Wraps a descriptor set to provide binding by name
class CDescriptorSetWrapper
{
public:
    explicit CDescriptorSetWrapper(const CParameterBlock& pb);

    const RHI::CDescriptorSet::Ref& Get() const { return DescriptorSet; }

    void BindBuffer(RHI::CBuffer::Ref buffer, size_t offset, size_t range, const std::string& name, uint32_t index);
    void BindImageView(RHI::CImageView::Ref imageView, const std::string& name, uint32_t index);
    void BindSampler(RHI::CSampler::Ref sampler, const std::string& name, uint32_t index);
    void BindBufferView(RHI::CBufferView::Ref bufferView, const std::string& name, uint32_t index);
    void SetDynamicOffset(size_t offset, const std::string& name, uint32_t index);

private:
    friend class CParameterBlock;
    const CParameterBlock& ParameterBlock;
    RHI::CDescriptorSet::Ref DescriptorSet;
};

class CParameterBlock
{
public:
    RHI::CDescriptorSetLayout::Ref GetDescriptorSetLayout() const;
    CDescriptorSetWrapper CreateDescriptorSet() const;
    const RHI::CDescriptorSetLayoutBinding& GetBinding(const std::string& name) const;

    // Called from Lua
    void AddBinding(const std::string& name,
                    uint32_t binding,
                    const std::string& type,
                    uint32_t count,
                    const std::string& stages);

    void CreateDescriptorLayout(const RHI::CDevice::Ref& device);

private:
    std::unordered_map<std::string, RHI::CDescriptorSetLayoutBinding> Bindings;
    RHI::CDescriptorSetLayout::Ref Layout;
};

class CPipelangContext;

class CPipelangLibrary
{
public:
    CPipelangLibrary(CPipelangContext* p, std::string sourceDir);

    void Parse();
    void PrecacheShaders();
    void RecreateDeviceResources();

    CVertexAttribs& GetVertexAttribs(const std::string& name);
    CParameterBlock& GetParameterBlock(const std::string& name);
    void AddVertexAttribs(const std::string& name, CVertexAttribs vertexAttribs);
    void AddParameterBlock(const std::string& name, CParameterBlock parameterBlock);

    void GetPipeline(RHI::CPipelineDesc& desc, const std::vector<std::string>& stages);

private:
    CPipelangContext* Parent;

    std::string SourceDir;
    std::unordered_map<std::string, CParameterBlock> ParameterBlocks;
    std::unordered_map<std::string, CVertexAttribs> VertexAttribDescs;
};

class CPipelangContext
{
public:
    explicit CPipelangContext(RHI::CDevice::Ref device = nullptr);

    CPipelangLibrary& CreateLibrary(std::string sourceDir);
    CPipelangLibrary& GetLibrary(const std::string& sourceDir);

    const RHI::CDevice::Ref& GetDevice() const { return Device; }
    void SetDevice(const RHI::CDevice::Ref& device) { Device = device; }

private:
    RHI::CDevice::Ref Device;
    std::unordered_map<std::string, std::unique_ptr<CPipelangLibrary>> LibraryByDir;
};

}
