#include "Scene.h"

namespace Foreground
{

CScene::CScene()
{
    RootNode = std::make_unique<CSceneNode>(this, nullptr);
    AccelStructure = std::make_unique<COctree>(100.0f);
}

CSceneNode* CScene::GetRootNode() const { return RootNode.get(); }

COctree* CScene::GetAccelStructure() const { return AccelStructure.get(); }

} /* namespace Foreground */
