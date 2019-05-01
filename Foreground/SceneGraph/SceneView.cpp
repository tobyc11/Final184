#include "SceneView.h"
#include <algorithm>

namespace Foreground
{

CSceneView::CSceneView(CSceneNode* node)
    : CameraNode(node)
{
    CameraNode->RetainDontKill();
}

CSceneView::~CSceneView()
{
    if (CameraNode)
        CameraNode->ReleaseDontKill();
}

void CSceneView::PrepareToRender()
{
    auto* scene = CameraNode->GetScene();
    auto* sceneAccel = scene->GetAccelStructure();
    auto camera = CameraNode->GetCamera();

    auto frustum = camera->GetFrustum().Transformed(CameraNode->GetWorldTransform());
    std::list<CSceneNode*> frustumCull;
    sceneAccel->Intersect(frustum, frustumCull);
    VisibleNodeList.resize(frustumCull.size());
    VisibleNodeList.insert(VisibleNodeList.begin(), frustumCull.begin(), frustumCull.end());

    for (CSceneNode* node : VisibleNodeList)
    {
        for (const auto& prim : node->GetPrimitives())
        {
            VisiblePrimModelMatrix.push_back(node->GetWorldTransform());
            VisiblePrimitiveList.push_back(prim.get());
        }
    }
}

void CSceneView::FrameFinished()
{
    VisibleNodeList.clear();
    VisiblePrimModelMatrix.clear();
    VisiblePrimitiveList.clear();
}

}
