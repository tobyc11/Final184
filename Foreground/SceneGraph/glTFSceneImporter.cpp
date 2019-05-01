#include "glTFSceneImporter.h"
#include "Primitive.h"
#include "tiny_gltf.h"

// RHI headers
#include <Resources.h>

#include <StringUtils.h>
#include <map>

namespace Foreground
{

static RHI::EFormat ConvertTinyglTFFormat(int component, int type)
{
    if (type == TINYGLTF_TYPE_SCALAR)
        switch (component)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return RHI::EFormat::R8_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return RHI::EFormat::R8_UINT;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return RHI::EFormat::R16_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return RHI::EFormat::R16_UINT;
        case TINYGLTF_COMPONENT_TYPE_INT:
            return RHI::EFormat::R32_SINT;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return RHI::EFormat::R32_SFLOAT;
        }
    if (type == TINYGLTF_TYPE_VEC2)
        switch (component)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return RHI::EFormat::R8G8_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return RHI::EFormat::R8G8_UINT;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return RHI::EFormat::R16G16_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return RHI::EFormat::R16G16_UINT;
        case TINYGLTF_COMPONENT_TYPE_INT:
            return RHI::EFormat::R32G32_SINT;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return RHI::EFormat::R32G32_SFLOAT;
        }
    if (type == TINYGLTF_TYPE_VEC3)
        switch (component)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return RHI::EFormat::R8G8B8_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return RHI::EFormat::R8G8B8_UINT;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return RHI::EFormat::R16G16B16_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return RHI::EFormat::R16G16B16_UINT;
        case TINYGLTF_COMPONENT_TYPE_INT:
            return RHI::EFormat::R32G32B32_SINT;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return RHI::EFormat::R32G32B32_SFLOAT;
        }
    if (type == TINYGLTF_TYPE_VEC4 || type == TINYGLTF_TYPE_MAT2)
        switch (component)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            return RHI::EFormat::R8G8B8A8_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return RHI::EFormat::R8G8B8A8_UINT;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            return RHI::EFormat::R16G16B16A16_SINT;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return RHI::EFormat::R16G16B16A16_UINT;
        case TINYGLTF_COMPONENT_TYPE_INT:
            return RHI::EFormat::R32G32B32A32_SINT;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return RHI::EFormat::R32G32B32A32_SFLOAT;
        }
    throw std::runtime_error("glTF accessor format unsupported");
}

class CglTFData
{
public:
    CglTFData(const tinygltf::Model& model, RHI::CDevice& device)
        : glTFModel(model)
        , Device(device)
    {
    }

    std::pair<RHI::CBuffer::Ref, uint32_t> GetBufferView(uint32_t index)
    {
        auto iter = BufferViews.find(index);
        if (iter != BufferViews.end())
            return iter->second;

        auto bufferIndex = glTFModel.bufferViews[index].buffer;
        auto size = glTFModel.bufferViews[index].byteLength;
        auto* data =
            glTFModel.buffers[bufferIndex].data.data() + glTFModel.bufferViews[index].byteOffset;
        auto usage = glTFModel.bufferViews[index].target == TINYGLTF_TARGET_ARRAY_BUFFER
            ? RHI::EBufferUsageFlags::VertexBuffer
            : RHI::EBufferUsageFlags::IndexBuffer;
        auto buf = Device.CreateBuffer(size, usage, data);
        BufferViews[index] = std::make_pair(buf, 0);
        return BufferViews[index];
    }

    RHI::CImageView::Ref GetImage(uint32_t index)
    {
        auto iter = Images.find(index);
        if (iter != Images.end())
            return iter->second;

        const auto& img = glTFModel.images[index];
        assert(img.component == 4);
        assert(img.bits == 8);

        auto image =
            Device.CreateImage2D(RHI::EFormat::R8G8B8A8_UNORM, RHI::EImageUsageFlags::Sampled,
                                 img.width, img.height, 1, 1, 1, img.image.data());
        RHI::CImageViewDesc viewDesc;
        viewDesc.Type = RHI::EImageViewType::View2D;
        viewDesc.Format = RHI::EFormat::R8G8B8A8_UNORM;
        viewDesc.Range.Set(0, 1, 0, 1);
        auto imageView = Device.CreateImageView(viewDesc, image);
        Images[index] = imageView;
        return imageView;
    }

    RHI::CSampler::Ref GetSampler(uint32_t index)
    {
        auto iter = Samplers.find(index);
        if (iter != Samplers.end())
            return iter->second;

        const auto& s = glTFModel.samplers[index];
        RHI::CSamplerDesc desc;

        if (s.minFilter % 2 == 0)
            desc.MinFilter = RHI::EFilter::Nearest;
        else
            desc.MinFilter = RHI::EFilter::Linear;

        if (s.magFilter % 2 == 0)
            desc.MagFilter = RHI::EFilter::Nearest;
        else
            desc.MagFilter = RHI::EFilter::Linear;

        if (s.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT)
            desc.AddressModeU = RHI::ESamplerAddressMode::Wrap;
        else if (s.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)
            desc.AddressModeU = RHI::ESamplerAddressMode::Clamp;
        else
            desc.AddressModeU = RHI::ESamplerAddressMode::Mirror;
        if (s.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT)
            desc.AddressModeV = RHI::ESamplerAddressMode::Wrap;
        else if (s.wrapT == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)
            desc.AddressModeV = RHI::ESamplerAddressMode::Clamp;
        else
            desc.AddressModeV = RHI::ESamplerAddressMode::Mirror;

        auto sampler = Device.CreateSampler(desc);
        Samplers[index] = sampler;
        return sampler;
    }

    std::shared_ptr<CBasicMaterial> GetMaterial(uint32_t index)
    {
        auto iter = Materials.find(index);
        if (iter != Materials.end())
            return iter->second;

        auto material = std::make_shared<CBasicMaterial>();
        material->SetAlbedo(tc::Vector4(0.5f, 0.5f, 0.8f, 1.0f));
        material->SetMetallic(0.0f);
        material->SetRoughness(1.0f);
        Materials[index] = material;
        return material;
    }

    std::vector<std::shared_ptr<CPrimitive>> GetMesh(uint32_t index)
    {
        auto iter = Meshes.find(index);
        if (iter != Meshes.end())
            return iter->second;

        const auto& m = glTFModel.meshes[index];
        std::vector<std::shared_ptr<CPrimitive>> prims;
        for (const auto& p : m.primitives)
        {
            static std::array<RHI::EPrimitiveTopology, 7> modeMap = {
                RHI::EPrimitiveTopology::PointList,    RHI::EPrimitiveTopology::LineList,
                RHI::EPrimitiveTopology::LineStrip,    RHI::EPrimitiveTopology::LineStrip,
                RHI::EPrimitiveTopology::TriangleList, RHI::EPrimitiveTopology::TriangleStrip,
                RHI::EPrimitiveTopology::TriangleFan
            };

            auto prim = std::make_shared<CPrimitive>();

            auto triMesh = std::make_shared<CTriangleMesh>();
            triMesh->SetPrimitiveTopology(modeMap[p.mode]);

            // Convert mesh attributes
            std::map<uint32_t, CBufferBinding> bufferBindingsByView;
            std::map<EAttributeType, CVertexAttribute> attributes;
            for (const auto& pair : p.attributes)
            {
                const auto& attrName = pair.first;
                const auto& accessorIndex = pair.second;
                const auto& accessor = glTFModel.accessors[accessorIndex];

                auto bindingIter = bufferBindingsByView.find(accessor.bufferView);
                if (bindingIter == bufferBindingsByView.end())
                {
                    CBufferBinding binding;
                    auto bufferView = GetBufferView(accessor.bufferView);
                    binding.Buffer = bufferView.first;
                    binding.Offset = bufferView.second;
                    binding.Stride = glTFModel.bufferViews[accessor.bufferView].byteStride;
                    if (binding.Stride == 0)
                        binding.Stride =
                            accessor.ByteStride(glTFModel.bufferViews[accessor.bufferView]);
                    bufferBindingsByView[accessor.bufferView] = binding;
                }

                EAttributeType attrType;
                if (attrName == "POSITION")
                    attrType = EAttributeType::Position;
                else if (attrName == "NORMAL")
                    attrType = EAttributeType::Normal;
                else if (attrName == "TANGENT")
                    attrType = EAttributeType::Tangent;
                else if (attrName == "TEXCOORD_0")
                    attrType = EAttributeType::TexCoord0;
                else if (attrName == "TEXCOORD_1")
                    attrType = EAttributeType::TexCoord1;
                else if (attrName == "COLOR_0")
                    attrType = EAttributeType::Color0;
                else if (attrName == "JOINTS_0")
                    attrType = EAttributeType::Joints0;
                else if (attrName == "WEIGHTS_0")
                    attrType = EAttributeType::Weights0;
                else
                    throw std::runtime_error("Encountered unsupported mesh attribute");
                CVertexAttribute attribute;
                attribute.Format = ConvertTinyglTFFormat(accessor.componentType, accessor.type);
                attribute.Offset = accessor.byteOffset;
                // Temporarily set this to buffer view id, and fix later once we know all bindings
                attribute.BindingIndex = accessor.bufferView;
                attributes.emplace(attrType, attribute);

                triMesh->SetElementCount(accessor.count);

                // Bounding box info also comes from position attribute
                if (attrType == EAttributeType::Position)
                {
                    tc::Vector3 min(accessor.minValues[0], accessor.minValues[1],
                                    accessor.minValues[2]);
                    tc::Vector3 max(accessor.maxValues[0], accessor.maxValues[1],
                                    accessor.maxValues[2]);
                    triMesh->SetBoundingBox(tc::BoundingBox(min, max));
                }
            }
            uint32_t index = 0;
            std::vector<CBufferBinding> bufferBindings;
            for (auto& pair : bufferBindingsByView)
            {
                bufferBindings.emplace_back(std::move(pair.second));
                for (auto& attribute : attributes)
                    if (attribute.second.BindingIndex == pair.first)
                        attribute.second.BindingIndex = index;
                index++;
            }
            bufferBindingsByView.clear();
            triMesh->SetAttributes(std::move(bufferBindings), std::move(attributes));

            // Convert index buffer if there exists one
            if (p.indices != -1)
            {
                const auto& accessor = glTFModel.accessors[p.indices];
                auto indexBufferView = GetBufferView(accessor.bufferView);
                triMesh->SetIndexBuffer(
                    indexBufferView.first,
                    ConvertTinyglTFFormat(accessor.componentType, accessor.type),
                    indexBufferView.second + accessor.byteOffset);
                triMesh->SetElementCount(accessor.count);
            }

            prim->SetShape(triMesh);
            if (p.material != -1)
                prim->SetMaterial(GetMaterial(p.material));
            prims.push_back(prim);
        }

        Meshes[index] = prims;
        return prims;
    }

private:
    const tinygltf::Model& glTFModel;
    RHI::CDevice& Device;
    std::map<uint32_t, std::pair<RHI::CBuffer::Ref, uint32_t>> BufferViews;
    std::map<uint32_t, RHI::CImageView::Ref> Images;
    std::map<uint32_t, RHI::CSampler::Ref> Samplers;
    std::map<uint32_t, std::shared_ptr<CBasicMaterial>> Materials;
    // A mesh is a collecton of primitives
    std::map<uint32_t, std::vector<std::shared_ptr<CPrimitive>>> Meshes;
};

struct CglTFNodeVisitor
{
public:
    CglTFNodeVisitor(std::shared_ptr<CScene> scene, const tinygltf::Model& model,
                     RHI::CDevice& device)
        : Scene(scene)
        , Model(model)
        , Data(model, device)
    {
    }

    void Visit(CSceneNode* parentNode, uint32_t index)
    {
        CSceneNode* node = parentNode->CreateChildNode();
        const auto& n = Model.nodes[index];
        node->SetName(n.name);
        if (!n.matrix.empty())
        {
            auto mat = tc::Matrix3x4(n.matrix[0], n.matrix[4], n.matrix[8], n.matrix[12],
                                     n.matrix[1], n.matrix[5], n.matrix[9], n.matrix[13],
                                     n.matrix[2], n.matrix[6], n.matrix[10], n.matrix[14]);
            node->SetTransform(mat);
        }
        else
        {
            if (!n.scale.empty())
                node->SetScale(tc::Vector3(n.scale[0], n.scale[1], n.scale[2]));
            if (!n.rotation.empty())
                node->SetRotation(
                    tc::Quaternion(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]));
            if (!n.translation.empty())
                node->SetPosition(
                    tc::Vector3(n.translation[0], n.translation[1], n.translation[2]));
        }

        if (n.mesh != -1)
        {
            auto meshPrimitives = Data.GetMesh(n.mesh);
            for (auto prim : meshPrimitives)
                node->AddPrimitive(std::move(prim));
        }

        for (auto childIndex : n.children)
            Visit(node, childIndex);
    }

private:
    std::shared_ptr<CScene> Scene;
    const tinygltf::Model& Model;
    CglTFData Data;
};

CglTFSceneImporter::CglTFSceneImporter(std::shared_ptr<CScene> scene, RHI::CDevice& device)
    : Scene(scene)
    , Device(device)
{
}

void CglTFSceneImporter::ImportFile(const std::string& path)
{
    using namespace tinygltf;

    Model model;
    TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret;
    if (tc::FStringUtils::EndsWith(path, "glb"))
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for binary glTF(.glb)
    else
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
        return;
    }

    const auto& scene = model.scenes[model.defaultScene];
    CglTFNodeVisitor visitor(Scene, model, Device);
    for (auto nodeId : scene.nodes)
        visitor.Visit(Scene->GetRootNode(), nodeId);
}

} /* namespace Foreground */
