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
    self->cameraNode->SetCamera(std::make_shared<CCamera>());

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

static void physics_tick(std::shared_ptr<CBehaviour> selfref, Game* game, double elapsedTime) {}

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

    ImGui::Render();
    self->renderPipeline->Render();
}

// ----------------------------------------------------------------------------
// Register handlers
// ----------------------------------------------------------------------------

MainBehaviour::MainBehaviour(CGameObject* root) {
    std::shared_ptr<CEvent> ev;

    ev = dynamic_pointer_cast<CEvent>(root->getFirstWithName("init"));
    if (ev)
    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)) },
                               (handlerFunc_t*)&init };

        handles.insert_or_assign(ev, handle);
    }

    ev = dynamic_pointer_cast<CEvent>(root->getFirstWithName("logic_tick"));
    if (ev)
    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)), std::type_index(typeid(double)) },
                               (handlerFunc_t*)&physics_tick };

        handles.insert_or_assign(ev, handle);
    }

    ev = dynamic_pointer_cast<CEvent>(root->getFirstWithName("render_tick"));
    if (ev)
    {
        EventHandler* handle =
            new EventHandler { VOID_TYPE,
                               { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                                 std::type_index(typeid(Game*)) },
                               (handlerFunc_t*)&render_tick };

        handles.insert_or_assign(ev, handle);
    }
    
}