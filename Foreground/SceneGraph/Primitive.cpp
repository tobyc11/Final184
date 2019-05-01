#include "Primitive.h"

namespace Foreground
{

tc::BoundingBox CPrimitive::GetBoundingBox() const { return Shape->GetBoundingBox(); }

std::shared_ptr<CBasicMaterial> CPrimitive::GetMaterial() const { return Material; }

void CPrimitive::SetMaterial(std::shared_ptr<CBasicMaterial> material) { Material = material; }

std::shared_ptr<CTriangleMesh> CPrimitive::GetShape() const { return Shape; }

void CPrimitive::SetShape(std::shared_ptr<CTriangleMesh> shape) { Shape = shape; }

}
