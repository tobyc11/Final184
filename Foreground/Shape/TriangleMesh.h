#pragma once
#include "Shader/VertexShaderCommon.h"
#include <BoundingBox.h>
#include <Format.h>
#include <ShaderModule.h>

namespace Foreground
{

struct CBufferBinding
{
    RHI::CBuffer::Ref Buffer;
    uint32_t Offset;
    uint32_t Stride;
};

struct CVertexAttribute
{
    RHI::EFormat Format;
    uint32_t Offset;
    uint32_t BindingIndex;
};

class CTriangleMesh
{
public:
    void SetAttributes(std::vector<CBufferBinding> bindings,
                       std::map<EAttributeType, CVertexAttribute> attributes)
    {
        assert(bindings.size() < 16);
        for (size_t i = 0; i < bindings.size(); i++)
            BufferBindings[i] = std::move(bindings[i]);
        Attributes = std::move(attributes);
    }

    void SetIndexBuffer(RHI::CBuffer::Ref buffer, RHI::EFormat format, uint32_t offset)
    {
        IndexBuffer = buffer;
        IndexBufferFormat = format;
        IndexBufferOffset = offset;
    }

    uint32_t GetElementCount() const { return ElementCount; }
    void SetElementCount(uint32_t count) { ElementCount = count; }

    RHI::EPrimitiveTopology GetPrimitiveTopology() const { return PrimTopology; }
    void SetPrimitiveTopology(RHI::EPrimitiveTopology t) { PrimTopology = t; }

    const tc::BoundingBox& GetBoundingBox() const { return BoundingBox; }
    void SetBoundingBox(tc::BoundingBox bb) { BoundingBox = std::move(bb); }

private:
    std::array<CBufferBinding, 16> BufferBindings;
    std::map<EAttributeType, CVertexAttribute> Attributes;

    RHI::CBuffer::Ref IndexBuffer;
    RHI::EFormat IndexBufferFormat;
    uint32_t IndexBufferOffset;

    uint32_t ElementCount;

    RHI::EPrimitiveTopology PrimTopology;
    tc::BoundingBox BoundingBox;
};

} /* namespace Foreground */
