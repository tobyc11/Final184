#include <utility>

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

        cout << "Shader " << VS_file << " : " << id << " set=" << res.Set
             << " binding=" << res.Binding << endl;
    }

    for (CPipelineResource res : PS->GetShaderResources())
    {
        string id = string(res.Name);

        resources.insert_or_assign(id, res);

        cout << "Shader " << PS_file << " : " << id << " set=" << res.Set
             << " binding=" << res.Binding << endl;
    }

    this->device = device;
}

CMaterial::~CMaterial()
{
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
        rpDesc.AddAttachment(renderTargetViews.at(id), EAttachmentLoadOp::Clear,
                             EAttachmentStoreOp::Store);

        if (t.isDepthStencil)
            rpDesc.Subpasses[0].SetDepthStencilAttachment(id);
        else
            rpDesc.Subpasses[0].AddColorAttachment(id);

        id++;
    }

    renderPass = device->CreateRenderPass(rpDesc);

    // 2. Create render pipeline

    // 2.1 Pipeline descriptions
    RHI::CPipelineDesc desc;
    desc.VS = VS;
    desc.PS = PS;
    desc.RasterizerState.CullMode = RHI::ECullModeFlags::None;
    desc.DepthStencilState.DepthEnable = false;
    desc.RenderPass = renderPass;
    desc.Subpass = 0;

    // 2.2 Pipeline descriptions for vertex attributes
    desc.VertexBindings.clear();
    desc.VertexBindings.reserve(inputBuffers.size());
    for (auto const& p : inputBuffers)
        desc.VertexBindings.emplace_back(p.second);

    desc.VertexAttributes.clear();
    desc.VertexAttributes.reserve(vertexAttributes.size());
    for (auto const& attr : vertexAttributes)
    {
        if (resources.find(attr.second.id) == resources.end())
        {
            cerr << "Vertex attribute " << attr.second.id << " not found in shaders" << endl;
            continue;
        }

        uint32_t location = resources[attr.second.id].Location;
        uint32_t bufferBinding = getInputBufferBinding(attr.second.buffer_name);

        if (bufferBinding == -1)
        {
            cerr << "Buffer " << attr.second.buffer_name << " for vertex attribute "
                 << attr.second.id << " not found" << endl;
            continue;
        }

        desc.VertexAttributes.push_back(
            { location, attr.second.format, (uint32_t)attr.second.offset, bufferBinding });
    }

    pipeline = device->CreateManagedPipeline(desc);
    DescriptorSets = pipeline->CreateDescriptorSets();
}

uint32_t CMaterial::getInputBufferBinding(std::string name) const
{
    if (inputBuffers.find(name) != inputBuffers.end())
        return inputBuffers.at(name).Binding;
    else
        return -1;
}

void CMaterial::setAttribute(std::string id, RHI::EFormat format, size_t offset,
                             std::string buffer_name)
{
    vertexAttributes.insert_or_assign(
        id, CMaterialNamedAttribute { id, format, offset, std::move(buffer_name) });
}

void CMaterial::setSampler(const std::string& id, RHI::CSampler::Ref obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources.at(id);
        DescriptorSets[r.Set]->BindSampler(std::move(obj), r.Binding, 0);
    }
}

void CMaterial::setImageView(const std::string& id, RHI::CImageView::Ref obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources.at(id);
        DescriptorSets[r.Set]->BindImageView(std::move(obj), r.Binding, 0);
    }
}

void CMaterial::setStruct(const std::string& id, size_t size, const void* obj)
{
    if (ctx)
    {
        CPipelineResource& r = resources.at(id);
        DescriptorSets[r.Set]->BindConstants(obj, size, r.Binding, 0);
    }
}

void CMaterial::setBuffer(const std::string& id, CBuffer& obj, size_t offset)
{
    if (ctx)
    {
        auto desc = inputBuffers.at(id);
        ctx->BindVertexBuffer(desc.Binding, obj, offset);
    }
}

const std::vector<RHI::CImageView::Ref>& CMaterial::getRTViews() const { return renderTargetViews; }

void CMaterial::beginRender(const RHI::CCommandList::Ref& c)
{
    PassCtx = c->CreateParallelRenderContext(renderPass, clearValues);
    ctx = PassCtx->CreateRenderContext(0);

    ctx->BindRenderPipeline(*pipeline->Get());
}

const RHI::IRenderContext::Ref& CMaterial::getContext() const { return ctx; }

void CMaterial::endRender()
{
    ctx->FinishRecording();
    ctx = nullptr;
    PassCtx->FinishRecording();
    PassCtx = nullptr;
}

void CMaterial::blit2d() const
{
    if (ctx)
    {
        for (uint32_t i = 0; i < DescriptorSets.size(); i++)
            if (DescriptorSets[i])
                ctx->BindRenderDescriptorSet(i, *DescriptorSets[i]);
        ctx->Draw(3, 1, 0, 0);
    }
}

}