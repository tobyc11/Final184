#pragma once
#include <ComputeContext.h>
#include <ManagedPipeline.h>
#include <ShaderModule.h>
#include <string>
#include <unordered_map>

namespace Foreground
{

class CComputePass
{
public:
    CComputePass(const std::string& shaderPath);

    void SetSampler(const std::string& id, RHI::CSampler::Ref obj);
    void SetImageView(const std::string& id, RHI::CImageView::Ref obj);
    void SetStruct(const std::string& id, size_t size, const void* obj);
    void Dispatch(RHI::IComputeContext& context, uint32_t x, uint32_t y, uint32_t z);

private:
    std::unordered_map<std::string, RHI::CPipelineResource> ResourceMap;
    RHI::CManagedPipeline::Ref Pipeline;
    std::vector<RHI::CDescriptorSet::Ref> DescSets;
};

}
