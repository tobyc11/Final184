#include "Material.h"
#include "Resources/ResourceManager.h"

#include <fstream>
#include <iostream>

using namespace std;

using namespace RHI;

namespace Foreground
{

static CShaderModule::Ref LoadSPIRV(CDevice::Ref device, const std::string& path)
{
    string filePath = CResourceManager::Get().FindShader(path);
    ifstream file(filePath.c_str(), ios::binary | ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return device->CreateShaderModule(buffer.size(), buffer.data());
    return {};
}

CMaterial::CMaterial(CDevice::Ref device, const string& VS_file, const string& PS_file)
{
    VS = LoadSPIRV(device, VS_file);
    PS = LoadSPIRV(device, PS_file);

    // Setup resources hashmap from reflections, it's kind of dirty right now
    for (CPipelineResource res : VS->GetShaderResources())
    {
        string id = string(res.Name);

        resources.insert_or_assign(id, res);

        cout << "Shader " << VS_file << " : " << id << endl;
    }

    for (CPipelineResource res : PS->GetShaderResources())
    {
        string id = string(res.Name);

        resources.insert_or_assign(id, res);

        cout << "Shader " << PS_file << " : " << id << endl;
    }

    this->device = device;
}

CMaterial::~CMaterial() {
    renderPass = nullptr;
    pipeline = nullptr;
    renderTargetViews.clear();
    clearValues.clear();
}

void CMaterial::createPipeline(int w, int h)
{
    // 0. Clear stuff
    renderPass = nullptr;
    pipeline = nullptr;
    renderTargetViews.clear();
    clearValues.clear();

    // 1. Create render pass
    cout << "Create render pass of " << w << "x" << h << endl;

    // 1.1 Create render targets
    for (auto& t : renderTargets)
    {
        CImageViewDesc viewDesc;
        viewDesc.Format = t.format;
        viewDesc.Type = EImageViewType::View2D;
        viewDesc.Range.Set(0, 1, 0, 1);
        renderTargetViews.push_back(device->CreateImageView(viewDesc, t.image));
        clearValues.push_back(t.clearValue);
    }

    // 1.2 Create RP desc
    CRenderPassDesc rpDesc;
    rpDesc.Subpasses.resize(1);
    rpDesc.Layers = 1;
    rpDesc.Width = w;
    rpDesc.Height = h;

    // 1.3 Bind render targets / attachments
    int id = 0;
    for (auto& t : renderTargets)
    {
        rpDesc.AddAttachment(renderTargetViews[id], EAttachmentLoadOp::Clear,
                             EAttachmentStoreOp::Store);

        if (t.isDepthStencil)
            rpDesc.Subpasses[0].SetDepthStencilAttachment(id);
        else
            rpDesc.Subpasses[0].AddColorAttachment(id);

        id++;
    }

    renderPass = device->CreateRenderPass(rpDesc);

    // 2. Create render pipeline
    RHI::CPipelineDesc desc;
    desc.VS = VS;
    desc.PS = PS;
    desc.RasterizerState.CullMode = RHI::ECullModeFlags::None;
    desc.DepthStencilState.DepthEnable = false;
    desc.RenderPass = renderPass;
    desc.Subpass = 0;
    pipeline = device->CreatePipeline(desc);
}

void CMaterial::setSampler(std::string id, RHI::CSampler::Ref obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources[id];
        ctx->BindSampler(*obj, r.Set, r.Binding, 0);
    }
}

void CMaterial::setImageView(std::string id, RHI::CImageView::Ref obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources[id];
        ctx->BindImageView(*obj, r.Set, r.Binding, 0);
    }
}

void CMaterial::setStruct(std::string id, size_t size, const void* obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources[id];
        ctx->BindConstants(obj, size, r.Set, r.Binding, 0);
    }
}

const std::vector<RHI::CImageView::Ref>& CMaterial::getRTViews() const { return renderTargetViews; }

void CMaterial::beginRender(IImmediateContext::Ref c)
{
    ctx = c;

    ctx->BeginRenderPass(*renderPass, clearValues);
    ctx->BindPipeline(*pipeline);
}

void CMaterial::endRender()
{
    if (ctx)
    {
        ctx->EndRenderPass();
    }
    ctx = nullptr;
}

void CMaterial::blit2d() const
{
    if (ctx)
    {
        ctx->Draw(3, 1, 0, 0);
    }
}

}