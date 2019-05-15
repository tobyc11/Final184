#include "ComputePass.h"
#include "ForegroundCommon.h"
#include "Resources/ResourceManager.h"
#include <Device.h>

#include <fstream>
#include <vector>

namespace Foreground
{

using namespace RHI;

static CShaderModule::Ref LoadSPIRV(CDevice::Ref device, const std::string& path)
{
    std::string filePath = CResourceManager::Get().FindShader(path);
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == -1)
        return {};

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return device->CreateShaderModule(buffer.size(), buffer.data());
    return {};
}

CComputePass::CComputePass(const std::string& shaderPath)
{
    CComputePipelineDesc desc;
    desc.CS = LoadSPIRV(RenderDevice, shaderPath);
    for (const auto& resource : desc.CS->GetShaderResources())
    {
        std::string id = resource.Name;
        ResourceMap.insert_or_assign(id, resource);
    }

    Pipeline = RenderDevice->CreateManagedComputePipeline(desc);
    DescSets = Pipeline->CreateDescriptorSets();
}

void CComputePass::SetSampler(const std::string& id, RHI::CSampler::Ref obj)
{
    const auto& r = ResourceMap.at(id);
    DescSets[r.Set]->BindSampler(obj, r.Binding, 0);
}

void CComputePass::SetImageView(const std::string& id, RHI::CImageView::Ref obj)
{
    const auto& r = ResourceMap.at(id);
    DescSets[r.Set]->BindImageView(obj, r.Binding, 0);
}

void CComputePass::SetStruct(const std::string& id, size_t size, const void* obj)
{
    const auto& r = ResourceMap.at(id);
    DescSets[r.Set]->BindConstants(obj, size, r.Binding, 0);
}

void CComputePass::Dispatch(IComputeContext& context, uint32_t x, uint32_t y, uint32_t z)
{
    for (uint32_t i = 0; i < DescSets.size(); i++)
        if (DescSets[i])
            context.BindComputeDescriptorSet(i, *DescSets[i]);
    context.BindComputePipeline(*Pipeline->Get());
    context.Dispatch(x, y, z);
}

}
