#include "Pipelang.h"
#include "ShaderCache.h"
#include <PathTools.h>

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <LuaBridge/Vector.h>

#include <algorithm>
#include <fstream>

namespace Pl
{

static void ExportClassesToLua(lua_State* L)
{
    using namespace luabridge;

    getGlobalNamespace(L)
        .beginNamespace("rt")
            .beginClass<CVertexAttribs>("CVertexAttribs")
                .addConstructor<void (*)(void)>()
                .addFunction("AddAttribute", &CVertexAttribs::AddAttribute)
            .endClass()
            .beginClass<CParameterBlock>("CParameterBlock")
                .addConstructor<void (*)(void)>()
                .addFunction("AddBinding", &CParameterBlock::AddBinding)
                .addFunction("SetSetIndex", &CParameterBlock::SetSetIndex)
                .addFunction("GetSetIndex", &CParameterBlock::GetSetIndex)
            .endClass()
            .beginClass<CPipelangLibrary>("CPipelangLibrary")
                .addFunction("AddParameterBlock", &CPipelangLibrary::AddParameterBlock)
                .addFunction("AddVertexAttribs", &CPipelangLibrary::AddVertexAttribs)
            .endClass()
            .beginClass<CPipelangContext>("CPipelangContext")
            .endClass()
        .endNamespace();
}

const std::map<uint32_t, CVertexAttribs::ESemantic>& CVertexAttribs::GetAttributesByLocation() const
{
    return AttribsByLocation;
}

void CVertexAttribs::AddAttribute(const std::string& name, uint32_t location)
{
    auto semantic = static_cast<ESemantic>(0);
    if (name == "Position")
        semantic = ESemantic::Position;
    if (name == "Normal")
        semantic = ESemantic::Normal;
    if (name == "Tangent")
        semantic = ESemantic::Tangent;
    if (name == "TexCoord0")
        semantic = ESemantic::TexCoord0;
    if (name == "TexCoord1")
        semantic = ESemantic::TexCoord1;
    if (name == "Color0")
        semantic = ESemantic::Color0;
    if (name == "Joints0")
        semantic = ESemantic::Joints0;
    if (name == "Weights0")
        semantic = ESemantic::Weights0;
    AttribsByLocation.emplace(location, semantic);
    assert(static_cast<uint32_t>(semantic) != 0);
}

RHI::CDescriptorSetLayout::Ref CParameterBlock::GetDescriptorSetLayout() const
{
    return Layout;
}

RHI::CDescriptorSet::Ref CParameterBlock::CreateDescriptorSet() const
{
    return GetDescriptorSetLayout()->CreateDescriptorSet();
}

const RHI::CDescriptorSetLayoutBinding& CParameterBlock::GetBinding(const std::string& name) const
{
    return Bindings.at(name);
}

void CParameterBlock::BindBuffer(const RHI::CDescriptorSet::Ref& ds, RHI::CBuffer::Ref buffer,
                                 size_t offset,
                                 size_t range,
                                 const std::string& name,
                                 uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->BindBuffer(std::move(buffer), offset, range, binding, index);
}

void CParameterBlock::BindConstants(const RHI::CDescriptorSet::Ref& ds,
                                    const void* data,
                                    size_t size,
                                    const std::string& name,
                                    uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->BindConstants(data, size, binding, index);
}

void CParameterBlock::BindImageView(const RHI::CDescriptorSet::Ref& ds, RHI::CImageView::Ref imageView, const std::string& name, uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->BindImageView(std::move(imageView), binding, index);
}

void CParameterBlock::BindSampler(const RHI::CDescriptorSet::Ref& ds, RHI::CSampler::Ref sampler, const std::string& name, uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->BindSampler(std::move(sampler), binding, index);
}

void CParameterBlock::BindBufferView(const RHI::CDescriptorSet::Ref& ds, RHI::CBufferView::Ref bufferView, const std::string& name, uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->BindBufferView(std::move(bufferView), binding, index);
}

void CParameterBlock::SetDynamicOffset(const RHI::CDescriptorSet::Ref& ds, size_t offset, const std::string& name, uint32_t index)
{
    uint32_t binding = GetBinding(name).Binding;
    ds->SetDynamicOffset(offset, binding, index);
}

void CParameterBlock::AddBinding(const std::string& name,
                                 uint32_t binding,
                                 const std::string& type,
                                 uint32_t count,
                                 const std::string& stages)
{
    RHI::CDescriptorSetLayoutBinding b{};
    b.Binding = binding;
    b.Count = count;
    for (const auto& ch : stages)
    {
        if (ch == 'V')
            b.StageFlags |= RHI::EShaderStageFlags::Vertex;
        if (ch == 'D')
            b.StageFlags |= RHI::EShaderStageFlags::TessellationControl;
        if (ch == 'H')
            b.StageFlags |= RHI::EShaderStageFlags::TessellationEvaluation;
        if (ch == 'G')
            b.StageFlags |= RHI::EShaderStageFlags::Geometry;
        if (ch == 'P')
            b.StageFlags |= RHI::EShaderStageFlags::Pixel;
        if (ch == 'C')
            b.StageFlags |= RHI::EShaderStageFlags::Compute;
    }
    static const std::unordered_map<std::string, RHI::EDescriptorType> typeMap = {
        {"sampler", RHI::EDescriptorType::Sampler},
        {"texture2D", RHI::EDescriptorType::Image},
        {"image2D", RHI::EDescriptorType::StorageImage},
        {"uniform", RHI::EDescriptorType::UniformBuffer},
        {"buffer", RHI::EDescriptorType::StorageBuffer},
    };
    b.Type = typeMap.at(type);
    Bindings.emplace(name, b);
}

void CParameterBlock::SetSetIndex(uint32_t index)
{
    SetIndex = index;
}

uint32_t CParameterBlock::GetSetIndex() const
{
    return SetIndex;
}

void CParameterBlock::CreateDescriptorLayout(const RHI::CDevice::Ref& device)
{
    if (!device)
    {
        Layout = nullptr;
        return;
    }

    if (Layout)
        return;

    std::vector<RHI::CDescriptorSetLayoutBinding> b;
    for (const auto& pair : Bindings)
        b.emplace_back(pair.second);
    Layout = device->CreateDescriptorSetLayout(b);
}

CPipelangLibrary::CPipelangLibrary(CPipelangContext* p, std::string sourceDir)
    : Parent(p), SourceDir(std::move(sourceDir))
{
}

void CPipelangLibrary::Parse()
{
    using namespace luabridge;
    int error;

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    ExportClassesToLua(L);

    {
        setGlobal(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal").c_str(), "internal_path");

        setGlobal(L, this, "library");
        setGlobal(L, Parent, "context");

        error = luaL_dofile(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal/sandbox.lua").c_str());
        if (error)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        error = luaL_dofile(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal/parser.lua").c_str());
        if (error)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }

        LuaRef parser = getGlobal(L, "parser");
        parser["add_all_interface_stages"]();

    }

    lua_close(L);

    RecreateDeviceResources();
}

void CPipelangLibrary::PrecacheShaders()
{

}

void CPipelangLibrary::RecreateDeviceResources()
{
    for (auto& pair : ParameterBlocks)
    {
        pair.second.CreateDescriptorLayout(Parent->GetDevice());
    }
}

CVertexAttribs& CPipelangLibrary::GetVertexAttribs(const std::string& name)
{
    return VertexAttribDescs[name];
}

CParameterBlock& CPipelangLibrary::GetParameterBlock(const std::string& name)
{
    return ParameterBlocks[name];
}

bool CPipelangLibrary::GetPipeline(RHI::CPipelineDesc& desc, const std::vector<std::string>& stages)
{
    // TODO: use 64bit hash key, not string concat
    std::string key;
    for (const std::string& s : stages)
    {
        key += "_";
        key += s;
    }
    desc.VS = Parent->GetShaderCache()->RetrieveShader(key + "_VS");
    desc.PS = Parent->GetShaderCache()->RetrieveShader(key + "_PS");
    auto plIter = Parent->GetPipelineLayoutCache().find(key);
    if (plIter != Parent->GetPipelineLayoutCache().end())
        desc.Layout = plIter->second;
    if (desc.VS && desc.PS && desc.Layout)
        return true;

    // Create pipeline layout from the specified parameter blocks
    std::vector<RHI::CDescriptorSetLayout::Ref> layouts;
    for (const std::string& s : stages)
    {
        auto pbIter = ParameterBlocks.find(s);
        if (pbIter != ParameterBlocks.end())
        {
            const auto& pb = pbIter->second;
            layouts.resize(std::max((uint32_t)layouts.size(), pb.GetSetIndex() + 1));
            layouts[pb.GetSetIndex()] = pb.GetDescriptorSetLayout();
        }
    }
    auto pipelineLayout = Parent->GetDevice()->CreatePipelineLayout(layouts);
    Parent->GetPipelineLayoutCache()[key] = pipelineLayout;
    desc.Layout = pipelineLayout;

    using namespace luabridge;
    int error;

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    ExportClassesToLua(L);

    {
        setGlobal(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal").c_str(), "internal_path");

        setGlobal(L, this, "library");
        setGlobal(L, Parent, "context");

        error = luaL_dofile(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal/sandbox.lua").c_str());
        if (error)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        error = luaL_dofile(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal/parser.lua").c_str());
        if (error)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        error = luaL_dofile(L, tc::FPathTools::Join(PIPELANG_SOURCE_DIR, "Internal/codegen.lua").c_str());
        if (error)
        {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }

        LuaRef codegen = getGlobal(L, "codegen");
        LuaRef success = codegen["make_pipeline"](stages);
        if (!success.cast<bool>())
        {
            lua_close(L);
            return false;
        }

        LuaRef result = codegen["result"];
        std::string vs = result["vs"];
        std::string ps = result["ps"];

        std::ofstream ofs(key + "_VS.glsl");
        ofs << vs;
        ofs.close();

        CShaderCompileEnvironment env;
        env.MainSourcePath = key + "_VS.glsl";
        env.ShaderStage = "vertex";

        desc.VS = Parent->GetShaderCache()->RetrieveOrCompileShader(key + "_VS", env);

        if (!desc.VS)
        {
            lua_close(L);
            return false;
        }

        ofs.open(key + "_PS.glsl");
        ofs << ps;
        ofs.close();

        env.MainSourcePath = key + "_PS.glsl";
        env.ShaderStage = "fragment";

        desc.PS = Parent->GetShaderCache()->RetrieveOrCompileShader(key + "_PS", env);

        if (!desc.PS)
        {
            lua_close(L);
            return false;
        }
    }

    lua_close(L);
    return true;
}

void CPipelangLibrary::AddVertexAttribs(const std::string& name, CVertexAttribs vertexAttribs)
{
    VertexAttribDescs[name] = std::move(vertexAttribs);
}

void CPipelangLibrary::AddParameterBlock(const std::string& name, CParameterBlock parameterBlock)
{
    ParameterBlocks[name] = std::move(parameterBlock);
}

CPipelangContext::CPipelangContext(RHI::CDevice::Ref device)
    : Device(std::move(device))
{
    ShaderCache = std::make_unique<CShaderCache>();
}

CPipelangContext::~CPipelangContext() = default;

CPipelangLibrary& CPipelangContext::CreateLibrary(std::string sourceDir)
{
    auto iter = LibraryByDir.find(sourceDir);
    if (iter == LibraryByDir.end())
    {
        auto ptr = std::make_unique<CPipelangLibrary>(this, sourceDir);
        LibraryByDir[sourceDir] = std::move(ptr);
        return *LibraryByDir[sourceDir];
    }
    throw std::runtime_error("Said library already exists");
}

CPipelangLibrary& CPipelangContext::GetLibrary(const std::string& sourceDir)
{
    return *LibraryByDir[sourceDir];
}

const std::unique_ptr<CShaderCache>& CPipelangContext::GetShaderCache() const
{
    return ShaderCache;
}

std::unordered_map<std::string, RHI::CPipelineLayout::Ref>& CPipelangContext::GetPipelineLayoutCache()
{
    return PipelineLayoutCache;
}

void CPipelangContext::NotifyDeviceChange()
{
    for (auto& iter : LibraryByDir)
        iter.second->RecreateDeviceResources();
    ShaderCache->SetDevice(Device);
}

}
