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

    CglTFSceneImporter importer(self->scene, *game->device);
    importer.ImportFile(CResourceManager::Get().FindFile("Models/Sponza.gltf"));

    self->renderPipeline =
        CForegroundBootstrapper::CreateRenderPipeline(game->swapChain, EForegroundPipeline::Mega);
    self->renderPipeline->SetSceneView(std::make_unique<CSceneView>(self->cameraNode));

    self->scene->UpdateAccelStructure();
}

// ----------------------------------------------------------------------------
// Event: logic_tick (__root_events)
// ----------------------------------------------------------------------------

static void physics_tick(std::shared_ptr<CBehaviour> selfref, Game* game, double elapsedTime)
{
    auto self = dynamic_pointer_cast<MainBehaviour>(selfref);

    float moveSpeed = 4;
    float moveDpos = moveSpeed / 120;

    float rotSpeed = 500;

    // Input handling
    float dYaw = -rotSpeed * game->control.getAnalog(Control::Analog::CameraYaw);
    float dPitch = rotSpeed * game->control.getAnalog(Control::Analog::CameraPitch);

    auto& camera = self->cameraNode;

    const tc::Quaternion& rotation = camera->GetWorldRotation();
    tc::Vector3 lookDirection = rotation * tc::Vector3(0, 0, -1);
    tc::Vector3 upDirection = rotation * tc::Vector3::UP;

    tc::Vector3 movementDir;
    float lookDotDown = lookDirection.DotProduct(tc::Vector3::DOWN);
    float lookDotDownSign = (0 < lookDotDown) - (lookDotDown < 0);
    if (abs(lookDotDown) > 0.5)
    {
        // Looking toward the poles
        movementDir = lookDotDownSign * tc::Vector3(upDirection.x, 0, upDirection.z).Normalized();
    }
    else
    {
        // Looking toward the center
        movementDir = tc::Vector3(lookDirection.x, 0, lookDirection.z).Normalized();
    }

    tc::Vector3 strafeDir = tc::Vector3(movementDir.z, 0, -movementDir.x);

    tc::Quaternion yaw(dYaw, tc::Vector3::UP);
    tc::Quaternion pitch(dPitch, strafeDir);
    tc::Quaternion dRot = yaw * pitch;

    if ((dRot * upDirection).DotProduct(tc::Vector3::UP) < 0)
    {
        // Camera upside down
        dRot.FromLookRotation(-lookDotDownSign * tc::Vector3::DOWN, lookDotDownSign * movementDir);
        camera->SetWorldRotation(dRot);
    }
    else
    {
        camera->Rotate(dRot, ETransformSpace::World);
    }

    camera->Translate(moveDpos * game->control.yAxis() * movementDir, ETransformSpace::World);
    camera->Translate(moveDpos * game->control.xAxis() * strafeDir, ETransformSpace::World);
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

    ImGui::Render();
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