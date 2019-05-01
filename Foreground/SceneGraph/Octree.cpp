#include "Octree.h"
#include "SceneNode.h"

namespace Foreground
{

COctree::COctree(float halfWidth)
{
    CellArray.reserve(512 + 1);
    CellArray.resize(1);

    auto& rootCell = CellArray[0];
    rootCell.Center = tc::Vector3::ZERO;
    rootCell.HalfSize = tc::Vector3(halfWidth, halfWidth, halfWidth);
    rootCell.ChildrenStartOffset = 0;
}

void COctree::InsertObject(CSceneNode* object)
{
    const auto& objBound = object->GetWorldBoundingBox();

    size_t currCell = 0;
    uint32_t currDepth = 0;
    while (true)
    {
        // Can fit in one of the children?
        const tc::Vector3& childSize = CellArray[currCell].HalfSize;
        if (currDepth < MaxDepth && objBound.Size() < childSize
            && objBound.Center() <= CellArray[currCell].Center + CellArray[currCell].HalfSize
            && objBound.Center() >= CellArray[currCell].Center - CellArray[currCell].HalfSize)
        {
            if (!HasChildren(CellArray[currCell]))
                AllocateChildCells(CellArray[currCell]);
            uint32_t x, y, z;
            if (objBound.Center().x > CellArray[currCell].Center.x)
                x = 1;
            else
                x = 0;
            if (objBound.Center().y > CellArray[currCell].Center.y)
                y = 1;
            else
                y = 0;
            if (objBound.Center().z > CellArray[currCell].Center.z)
                z = 1;
            else
                z = 0;
            currCell = CellArray[currCell].ChildrenStartOffset + (x | y << 1 | z << 2);
            currDepth++;
            continue;
        }

        // Otherwise just store it here
        CellArray[currCell].ObjectList.insert(object);
        ObjectToNode[object] = currCell;
        return;
    }
}

void COctree::EraseObject(CSceneNode* object)
{
    size_t cellIndex = ObjectToNode[object];
    CellArray[cellIndex].ObjectList.erase(object);
}

void COctree::UpdateObject(CSceneNode* object)
{
    // Do this more efficiently?
    EraseObject(object);
    InsertObject(object);
}

void COctree::Intersect(const tc::Ray& ray, std::list<CSceneNode*>& result)
{
    throw "unimplemented";
}

void COctree::Intersect(const tc::Frustum& frustum, std::list<CSceneNode*>& result)
{
    Intersect(0, frustum, result);
}

void COctree::AllocateChildCells(COctreeCell& cell)
{
#define GetChild(node, offset) CellArray[node.ChildrenStartOffset + offset]

    // Potential data race?
    cell.ChildrenStartOffset = CellArray.size();
    CellArray.resize(CellArray.size() + 8);
    tc::Vector3 quaterSize = cell.HalfSize / 2.0f;
    for (uint32_t x = 0; x < 2; x++)
        for (uint32_t y = 0; y < 2; y++)
            for (uint32_t z = 0; z < 2; z++)
            {
                tc::Vector3 mask((float)x * 2 - 1, (float)y * 2 - 1, (float)z * 2 - 1);
                auto off = x | y << 1 | z << 2;
                GetChild(cell, off).Center = cell.Center + quaterSize * mask;
                GetChild(cell, off).HalfSize = cell.HalfSize / 2.0f;
                GetChild(cell, off).ChildrenStartOffset = 0;
            }
}

bool COctree::HasChildren(const COctreeCell& cell) { return cell.ChildrenStartOffset != 0; }

void COctree::Intersect(size_t cell, const tc::Frustum& frustum, std::list<CSceneNode*>& result)
{
    tc::BoundingBox cellBb(CellArray[cell].Center - CellArray[cell].HalfSize,
                           CellArray[cell].Center + CellArray[cell].HalfSize);
    if (frustum.IsInsideFast(cellBb) != tc::OUTSIDE)
    {
        // Loop over objects and test
        for (CSceneNode* object : CellArray[cell].ObjectList)
            if (frustum.IsInsideFast(object->GetWorldBoundingBox()) != tc::OUTSIDE)
                result.push_back(object);

        for (uint32_t x = 0; x < 2; x++)
            for (uint32_t y = 0; y < 2; y++)
                for (uint32_t z = 0; z < 2; z++)
                {
                    auto off = x | y << 1 | z << 2;
                    Intersect(cell + off, frustum, result);
                }
    }
}

} /* namespace Foreground */
