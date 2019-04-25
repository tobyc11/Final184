#pragma once
#include <Frustum.h>
#include <Ray.h>
#include <Vector3.h>
#include <list>
#include <set>
#include <unordered_map>
#include <vector>

namespace Foreground
{

class CSceneNode;

struct COctreeCell
{
    std::set<CSceneNode*> ObjectList;
    tc::Vector3 Center;
    tc::Vector3 HalfSize;
    size_t ChildrenStartOffset = 0;
};

// We implement a loose octree
// http://www.tulrich.com/geekstuff/partitioning.html
class COctree
{
public:
    COctree(float halfWidth);

    void InsertObject(CSceneNode* object);
    void EraseObject(CSceneNode* object);
    void UpdateObject(CSceneNode* object);

    void Intersect(const tc::Ray& ray, std::list<CSceneNode*>& result);
    void Intersect(const tc::Frustum& frustum, std::list<CSceneNode*>& result);

protected:
    void AllocateChildCells(COctreeCell& cell);
    bool HasChildren(const COctreeCell& cell);

private:
    static const size_t MaxDepth = 7;
    std::vector<COctreeCell> CellArray;
    std::unordered_map<CSceneNode*, size_t> ObjectToNode;
};

} /* namespace Foreground */
