/* ============================================================================
 * MainBehaviour.cpp
 * ============================================================================
 *
 * The main & entry behaviour of the game
 * ----------------------------------------------------------------------------
 */

#include "MainBehaviour.h"

#include <iostream>

using namespace std;

// ----------------------------------------------------------------------------
// Event: init (__root_events)
// ----------------------------------------------------------------------------

static void init(std::shared_ptr<CBehaviour> selfref, Game* game)
{
    auto self = dynamic_pointer_cast<MainBehaviour>(selfref);

    cout << "\n\n\n\nInit from main behaviour!" << endl;

    // Load model
    self->scene = std::make_shared<CScene>();
    self->cameraNode = self->scene->GetRootNode()->CreateChildNode();
    self->cameraNode->SetName("CameraNode");
    self->camera = std::make_shared<CCamera>();
    self->camera->SetAspectRatio((float)game->windowWidth / (float)game->windowHeight);
    self->cameraNode->SetCamera(self->camera);
    self->cameraNode->Translate(tc::Vector3(0, 0.5, 0), ETransformSpace::World);

    self->pointLightNode = self->scene->GetRootNode()->CreateChildNode();
    self->pointLightNode->SetName("pointLightNode");
    self->lightPoint = std::make_shared<CLight>(tc::Color(0.1, 1.0, 0.6), 1.0);
    self->pointLightNode->AddLight(self->lightPoint);
    self->pointLightNode->Translate(tc::Vector3(0.0, 2.0, 0.0), ETransformSpace::World);
    
    self->directionalLightNode = self->scene->GetRootNode()->CreateChildNode();
    self->directionalLightNode->SetName("directionalLightNode");
    self->lightDirectional = std::make_shared<CLight>(tc::Color(1.0, 0.95, 0.87), 4.0, ELightType::Directional);
    self->directionalLightNode->AddLight(self->lightDirectional);
    self->directionalLightNode->Rotate(tc::Quaternion(-70.0, -30.0, 50.0), ETransformSpace::World);
    self->directionalLightNode->Translate(tc::Vector3(0.0, 16.0, 0.0), ETransformSpace::World);
    self->shadowCamera = std::make_shared<CCamera>(true);
    self->shadowCamera->SetFarClip(20.0);
    self->shadowCamera->SetNearClip(1.0);
    self->shadowCamera->SetAspectRatio(1.0);
    self->shadowCamera->SetMagX(16.0);
    self->shadowCamera->SetMagY(16.0);
    self->directionalLightNode->SetCamera(self->shadowCamera);

    self->voxelizerCamNode = self->scene->GetRootNode()->CreateChildNode();
    self->voxelizerCamNode->SetName("voxelizerCamNode");
    self->voxelizerCamNode->Rotate(tc::Quaternion(-90.0, 0.0, 0.0), ETransformSpace::World);
    self->voxelizerCamNode->Translate(tc::Vector3(0.0, 16.0, 0.0), ETransformSpace::World);
    self->voxelizerCamera = std::make_shared<CCamera>(true);
    self->voxelizerCamera->SetFarClip(20.0);
    self->voxelizerCamera->SetNearClip(1.0);
    self->voxelizerCamera->SetAspectRatio(1.0);
    self->voxelizerCamera->SetMagX(16.0);
    self->voxelizerCamera->SetMagY(16.0);
    self->voxelizerCamNode->SetCamera(self->voxelizerCamera);

    CglTFSceneImporter importer(self->scene, *game->device);
    importer.ImportFile(CResourceManager::Get().FindFile("Models/Sponza.gltf"));

    self->renderPipeline =
        CForegroundBootstrapper::CreateRenderPipeline(game->swapChain, EForegroundPipeline::Mega);
    self->renderPipeline->SetSceneView(std::make_unique<CSceneView>(self->cameraNode),
                                       std::make_unique<CSceneView>(self->directionalLightNode),
                                       std::make_unique<CSceneView>(self->voxelizerCamNode));

    self->scene->UpdateAccelStructure();
}

// ----------------------------------------------------------------------------
// Event: logic_tick (__root_events)
// ----------------------------------------------------------------------------

static void physics_tick(std::shared_ptr<CBehaviour> selfref, Game* game, double elapsedTime)
{
    auto self = dynamic_pointer_cast<MainBehaviour>(selfref);

    self->directionalLightNode->SetRotation(
        tc::Quaternion(-80.0, fmod(elapsedTime * 6.0, 360.0), 0.0));

    float moveSpeed = 4;
    float moveDpos = moveSpeed / 120;

    float rotSpeed = 0.002;

    // Input handling
    float dYaw = -rotSpeed * game->control.getAnalog(Control::Analog::CameraYaw);
    float dPitch = -rotSpeed * game->control.getAnalog(Control::Analog::CameraPitch);

    auto& camera = self->cameraNode;

    const tc::Quaternion& rotation = camera->GetWorldRotation();
    tc::Vector3 lookDirection = rotation * tc::Vector3::FORWARD;

    tc::Vector3 movementDir;
    float lookDotDown = lookDirection.DotProduct(tc::Vector3::DOWN);
    if (abs(lookDotDown) > 0.5)
    {
        // Looking toward the poles
        tc::Vector3 upDirection = rotation * tc::Vector3::UP;
        float lookDotDownSign = (0 < lookDotDown) - (lookDotDown < 0);

        movementDir = lookDotDownSign * tc::Vector3(upDirection.x, 0, upDirection.z).Normalized();
    }
    else
    {
        // Looking toward the center
        movementDir = tc::Vector3(lookDirection.x, 0, lookDirection.z).Normalized();
    }

    tc::Vector3 strafeDir = tc::Vector3(movementDir.z, 0, -movementDir.x);

    camera->Translate(moveDpos * game->control.zAxis() * movementDir, ETransformSpace::World);
    camera->Translate(moveDpos * game->control.xAxis() * strafeDir, ETransformSpace::World);
    camera->Translate(moveDpos * game->control.yAxis() * tc::Vector3::UP, ETransformSpace::World);

    if (abs(dYaw) < FLT_EPSILON && abs(dPitch) < FLT_EPSILON) {
        // Don't apply any rotations if we're not moving the camera
        // Helps avoid some of the instability of quaternion dynamics
        return;
    }

    float currYaw = atan2(movementDir.x, movementDir.z);
    float newYaw = dYaw + currYaw;
    float currPitch = asin(lookDirection.y);
    float newPitch = clamp(-tc::M_HALF_PI, currPitch + dPitch, tc::M_HALF_PI);

    tc::Vector3 pitched_forward(0, -sin(newPitch), -cos(newPitch));
    tc::Vector3 pitched_up(0, -pitched_forward.z, pitched_forward.y);
    tc::Vector3 pitched_strafe = -pitched_forward.CrossProduct(pitched_up);

    tc::Matrix3 pitched_space(pitched_strafe, pitched_up, pitched_forward);

    tc::Matrix3 rotateYaw(
         cos(newYaw), 0, sin(newYaw),
         0,           1,           0,
        -sin(newYaw), 0, cos(newYaw)
    );

    camera->SetWorldRotation(tc::Quaternion(rotateYaw * pitched_space));
}

// ----------------------------------------------------------------------------
// Event: render_tick (__root_events)
// ----------------------------------------------------------------------------

static void render_tick(std::shared_ptr<CBehaviour> selfref, Game* game)
{
    auto self = dynamic_pointer_cast<MainBehaviour>(selfref);

    // Draw GUI stuff
    CRHIImGuiBackend::NewFrame();
    ImGui_ImplSDL2_NewFrame(game->window);
    ImGui::NewFrame();

    self->scene->ShowInspectorImGui();
    CGameObject::ShowInspectorImGui(game->root);

    self->renderPipeline->Render();
}

// ----------------------------------------------------------------------------
// Event: resize (__root_events)
// ----------------------------------------------------------------------------

static void resize(std::shared_ptr<CBehaviour> selfref, Game* game)
{
    auto self = dynamic_pointer_cast<MainBehaviour>(selfref);

    self->renderPipeline->Resize();

    self->camera->SetAspectRatio((float)game->windowWidth / (float)game->windowHeight);
}

// ----------------------------------------------------------------------------
// Register handlers
// ----------------------------------------------------------------------------

MainBehaviour::MainBehaviour(CGameObject* root)
{
    CGameObject* root_events = root->getFirstWithName("__root_events").get();

    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)) },
                               (handlerFunc_t*)&init };

        registerFirstNamedEvent(root_events, "init", handle);
    }

    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)), std::type_index(typeid(double)) },
                               (handlerFunc_t*)&physics_tick };

        registerFirstNamedEvent(root_events, "logic_tick", handle);
    }

    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)) },
                               (handlerFunc_t*)&render_tick };

        registerFirstNamedEvent(root_events, "render_tick", handle);
    }

    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)) },
                               (handlerFunc_t*)&resize };

        registerFirstNamedEvent(root_events, "resize", handle);
    }
}