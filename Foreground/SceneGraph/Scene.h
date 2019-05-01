#pragma once
#include "Octree.h"
#include "SceneNode.h"

namespace Foreground
{

class CScene
{
public:
    CScene();

    CSceneNode* GetRootNode() const;
    COctree* GetAccelStructure() const;
    void UpdateAccelStructure() const;

    void ShowInspectorImGui();

private:
    std::unique_ptr<CSceneNode> RootNode;
    std::unique_ptr<COctree> AccelStructure;
};

} /* namespace Foreground */
