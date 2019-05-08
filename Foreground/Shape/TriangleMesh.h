#pragma once
#include <BoundingBox.h>
#include <Format.h>
#include <Pipeline.h>
#include <RenderContext.h>
#include <ShaderModule.h>
#include <Pipelang.h>

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
                       std::map<Pl::CVertexAttribs::ESemantic, CVertexAttribute> attributes)
    {
        assert(bindings.size() < 16);
        for (size_t i = 0; i < bindings.size(); i++)
            BufferBindings[i] = std::move(bindings[i]);
        Attributes = std::move(attributes);
    }

    void PipelineSetVertexInputDesc(RHI::CPipelineDesc& desc,
                                    const std::map<uint32_t, Pl::CVertexAttribs::ESemantic>& locationMap) const
    {
        for (const auto& pair : locationMap)
        {
            auto iter = Attributes.find(pair.second);
            if (iter == Attributes.end())
                printf("Warning: required attribute not found in mesh.");
            else
                desc.VertexAttribFormat(pair.first, iter->second.Format, iter->second.Offset,
                                        iter->second.BindingIndex);
        }
        for (uint32_t i = 0; i < static_cast<uint32_t>(BufferBindings.size()); i++)
            if (BufferBindings[i].Buffer)
            {
                desc.VertexBinding(i, BufferBindings[i].Stride);
            }
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

    void Draw(RHI::IRenderContext& context) const
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(BufferBindings.size()); i++)
            if (BufferBindings[i].Buffer)
            {
                context.BindVertexBuffer(i, *BufferBindings[i].Buffer, BufferBindings[i].Offset);
            }
        if (IndexBuffer)
        {
            context.BindIndexBuffer(*IndexBuffer, IndexBufferOffset, IndexBufferFormat);
            context.DrawIndexed(ElementCount, 1, 0, 0, 0);
        }
        else
        {
            context.Draw(ElementCount, 1, 0, 0);
        }
    }

private:
    std::array<CBufferBinding, 16> BufferBindings;
    std::map<Pl::CVertexAttribs::ESemantic, CVertexAttribute> Attributes;

    RHI::CBuffer::Ref IndexBuffer;
    RHI::EFormat IndexBufferFormat;
    uint32_t IndexBufferOffset;

    uint32_t ElementCount;

    RHI::EPrimitiveTopology PrimTopology;
    tc::BoundingBox BoundingBox;
};

} /* namespace Foreground */
