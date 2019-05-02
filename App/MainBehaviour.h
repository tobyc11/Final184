#pragma once

#include <Components/Behaviour.h>
#include <Components/GameObject.h>
#include <Components/Event.h>

#include <SceneGraph/Scene.h>
#include <SceneGraph/glTFSceneImporter.h>

#include <ForegroundBootstrapper.h>
#include <Renderer/MegaPipeline.h>

#include "Game.h"

namespace Foreground
{

class MainBehaviour : public CBehaviour
{
public:
    std::shared_ptr<CScene> scene;
    std::shared_ptr<CMegaPipeline> renderPipeline;
    CSceneNode* cameraNode;
    std::shared_ptr<CCamera> camera;

    MainBehaviour(CGameObject* root);
};

}