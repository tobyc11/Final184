#include "SceneView.h"
#include <algorithm>
#include <stack>

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

    // Lock in the constants
    ViewConstants.CameraPos = tc::Vector4(CameraNode->GetWorldPosition(), 1.0f);
    ViewConstants.ViewMat = CameraNode->GetWorldTransform().Inverse().ToMatrix4().Transpose();
    ViewConstants.ProjMat = camera->GetMatrix().Transpose();
    ViewConstants.InvProj = camera->GetMatrix().Inverse().Transpose();

    if (0)
    {
        // Frustum culling enabled
        auto frustum = camera->GetFrustum().Transformed(CameraNode->GetWorldTransform());
        std::list<CSceneNode*> frustumCull;
        sceneAccel->Intersect(frustum, frustumCull);
        VisibleNodeList.reserve(frustumCull.size());
        VisibleNodeList.insert(VisibleNodeList.begin(), frustumCull.begin(), frustumCull.end());
    }
    else
    {
        std::stack<CSceneNode*> dfsStack;
        dfsStack.push(scene->GetRootNode());
        while (!dfsStack.empty())
        {
            CSceneNode* node = dfsStack.top();
            dfsStack.pop();
            VisibleNodeList.push_back(node);
            for (CSceneNode* child : node->GetChildren())
            {
                dfsStack.push(child);
            }
        }
    }

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
