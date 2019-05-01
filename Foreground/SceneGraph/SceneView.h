#pragma once
#include "Scene.h"
#include <LangUtils.h>

namespace Foreground
{

class CSceneView : public tc::FNonCopyable
{
public:
    CSceneView(CSceneNode* node);
    ~CSceneView();

    CSceneNode* GetCameraNode() const { return CameraNode; }

    // Called by the conductor/mega pipeline or whatever?
    void PrepareToRender();
    void FrameFinished();

    const std::vector<CSceneNode*>& GetVisibleNodeList() const { return VisibleNodeList; }
    const std::vector<tc::Matrix3x4>& GetVisiblePrimModelMatrix() const
    {
        return VisiblePrimModelMatrix;
    }
    const std::vector<CPrimitive*>& GetVisiblePrimitiveList() const { return VisiblePrimitiveList; }

private:
    // A Scene node that holds a camera
    CSceneNode* CameraNode = nullptr;

    // Frame render data
    std::vector<CSceneNode*> VisibleNodeList;
    std::vector<tc::Matrix3x4> VisiblePrimModelMatrix;
    std::vector<CPrimitive*> VisiblePrimitiveList;
};

}
