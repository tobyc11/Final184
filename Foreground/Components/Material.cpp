#include "Material.h"
#include "Resources/ResourceManager.h"

#include <fstream>
#include <iostream>

#include <spirv_cross.hpp>

using namespace std;

namespace Foreground
{

vector<char> loadRawSpriv(string path)
{
    string filePath = CResourceManager::Get().FindShader(path);
    ifstream file(filePath.c_str(), ios::binary | ios::ate);
    streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return buffer;

    return {};
}

CMaterial::CMaterial(const string& VS_file, const string& PS_file) {
    vector<char> spriv_vs = loadRawSpriv(VS_file);
    vector<char> spriv_ps = loadRawSpriv(PS_file);

    spirv_cross::Compiler comp(move(spirv));

    auto active = comp.get_active_interface_variables();
    ShaderResources res = comp.get_shader_resources(active);
    comp.set_enabled_interface_variables(move(active));

    // TODO do these stuff properlly
    for (Resource r : res.separate_images)
    {
        
    }

    for (Resource r : res.separate_samplers)
    {
    }
}

void CMaterial::setSampler(std::string id, RHI::CSampler::Ref obj) {}

void CMaterial::setImageView(std::string id, RHI::CImageView::Ref obj) {}

void CMaterial::beginRender() const {}

void CMaterial::endRender() const {}

void CMaterial::blit2d() const {}

}