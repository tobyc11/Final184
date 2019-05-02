/* ============================================================================
 * Game.cpp
 * ============================================================================
 *
 * 1. The base for the engine's executable & serves as entry point.
 * 2. Handles the window system.
 * 3. Provide basic events
 * 4. Create root GameObject node
 * 5. Calls the mainBehaviour
 * ----------------------------------------------------------------------------
 */

#include "Game.h"

#include "MainBehaviour.h"

// ----------------------------------------------------------------------------
// Native platform window handling
// ----------------------------------------------------------------------------

// Initialize render platform
int platformInit(std::vector<std::string> cmd_args, Game& game)
{
    CResourceManager::Get().Init();

    // Initialize devices & windows
    game.device = CInstance::Get().CreateDevice(EDeviceCreateHints::Discrete);
    SDL_Init(SDL_INIT_VIDEO);
    game.window = SDL_CreateWindow("RHI Triangle Demo", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, 640, 480,
                                   SDL_WINDOW_RESIZABLE
#if TC_OS == TC_OS_MAC_OS_X
                                       | SDL_WINDOW_VULKAN
#endif
    );

    if (game.window == nullptr)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(game.window, &wmInfo);

    game.windowWidth = 640;
    game.windowHeight = 480;

    // Bind presentation surface to window
#if TC_OS == TC_OS_WINDOWS_NT
    game.surfaceDesc.Win32.Instance = wmInfo.info.win.hinstance;
    game.surfaceDesc.Win32.Window = wmInfo.info.win.window;
    game.surfaceDesc.Type = EPresentationSurfaceDescType::Win32;
#elif TC_OS == TC_OS_LINUX
    game.surfaceDesc.Linux.xconn = XGetXCBConnection(wmInfo.info.x11.display);
    game.surfaceDesc.Linux.window = wmInfo.info.x11.window;
    game.surfaceDesc.Type = EPresentationSurfaceDescType::Linux;
#elif TC_OS == TC_OS_MAC_OS_X
    auto nativeDevice = RHI::GetNativeDevice(game.device);
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SDL_Vulkan_CreateSurface(game.window, nativeDevice.Instance, &surface);
    game.surfaceDesc.Type = EPresentationSurfaceDescType::Vulkan;
    game.surfaceDesc.Vulkan.Surface = surface;
#endif

    game.swapChain = game.device->CreateSwapChain(game.surfaceDesc, EFormat::R8G8B8A8_UNORM);

    CForegroundBootstrapper::Init(game.device);

    game.isRunning.store(true);

    return 0;
}

// Initialize GUI component
void imguiInit(std::vector<std::string> cmd_args, Game& game)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForVulkan(game.window);
}

// Sync & shutdown
void shutdown(Game& game)
{
    CForegroundBootstrapper::Shutdown();
    SDL_DestroyWindow(game.window);
    SDL_Quit();
    CResourceManager::Get().Shutdown();
}

// Resize
void resize(Game& game, std::shared_ptr<CBehaviour> behaviour) {
    handlerFunc_t* handle = behaviour->getTrigger(game.event_resize);
    if (handle)
    {
        std::cout << "resize event" << std::endl;
        typedef void (*event_init_function)(std::shared_ptr<CBehaviour>, Game*);
        reinterpret_cast<event_init_function>(handle)(behaviour, &game);
    }
}

// ----------------------------------------------------------------------------
// Events & Behaviours setup
// ----------------------------------------------------------------------------

std::shared_ptr<CGameObject> createRootEvents(Game& game)
{
    std::shared_ptr<CGameObject> root_events(new CEmptyGameObject("__root_events"));

    EventHandler* event_init_handle =
        new EventHandler { VOID_TYPE,
                           { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                             std::type_index(typeid(Game*)) },
                           NULL };
    game.event_init = std::shared_ptr<CEvent>(new CEvent("init", event_init_handle));
    root_events->addChild(game.event_init);

    EventHandler* event_logic_tick_handle =
        new EventHandler { VOID_TYPE,
                           { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                             std::type_index(typeid(Game*)), std::type_index(typeid(double)) },
                           NULL };
    game.event_logic_tick =
        std::shared_ptr<CEvent>(new CEvent("logic_tick", event_logic_tick_handle));
    root_events->addChild(game.event_logic_tick);

    EventHandler* event_render_tick_handle =
        new EventHandler { VOID_TYPE,
                           { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                             std::type_index(typeid(Game*)) },
                           NULL };
    game.event_render_tick =
        std::shared_ptr<CEvent>(new CEvent("render_tick", event_render_tick_handle));
    root_events->addChild(game.event_render_tick);

    EventHandler* event_resize_handle =
        new EventHandler { VOID_TYPE,
                           { std::type_index(typeid(std::shared_ptr<CBehaviour>)),
                             std::type_index(typeid(Game*)) },
                           NULL };
    game.event_resize = std::shared_ptr<CEvent>(new CEvent("resize", event_resize_handle));
    root_events->addChild(game.event_resize);

    return root_events;
}

// ----------------------------------------------------------------------------
// Physics & game logic thread
// ----------------------------------------------------------------------------
using namespace std::chrono_literals;

void game_logic(Game* game, std::shared_ptr<CBehaviour> main_behaviour)
{
    const int tickRate = 120;
    const double tickInterval = 1.0 / (double)tickRate;

    uint64_t ticksElapsed = 0;
    uint64_t lastIntervalTicks = 0;

    double physicsTickRate = 0.0;
    double elapsedTime = 0.0;

    auto timeStart = std::chrono::steady_clock::now();

    double tickRateLastUpdated = -1.0;

    while (game->isRunning.load())
    {
        auto timeTarget =
            timeStart + (ticksElapsed + 1) * std::chrono::duration<double>(tickInterval);
        elapsedTime = (std::chrono::steady_clock::now() - timeStart).count() * 1.0e-9;

        if (elapsedTime > tickRateLastUpdated + 1.0)
        {
            physicsTickRate =
                (double)(ticksElapsed - lastIntervalTicks) / (elapsedTime - tickRateLastUpdated);
            lastIntervalTicks = ticksElapsed;
            tickRateLastUpdated = elapsedTime;
        }

        // Trigger physics tick events
        handlerFunc_t* handle = main_behaviour->getTrigger(game->event_logic_tick);
        if (handle)
        {
            typedef void (*event_init_function)(std::shared_ptr<CBehaviour>, Game*, double);
            reinterpret_cast<event_init_function>(handle)(main_behaviour, game, elapsedTime);
        }

        ticksElapsed++;

        std::this_thread::sleep_until(timeTarget);
    }
}

// ----------------------------------------------------------------------------
// main (Entry)
// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::vector<std::string> cmd_args = {};

    for (int i = 0; i < argc; i++)
    {
        cmd_args.emplace_back(argv[i]);
    }

    Game game;

    int rc = platformInit(cmd_args, game);

    if (rc)
        return rc;

    imguiInit(cmd_args, game);

    game.root = new CEmptyGameObject("__game_root");

    game.root->addChild(createRootEvents(game));

    // Create Main Behaviour
    std::shared_ptr<CBehaviour> main_behaviour(new MainBehaviour(game.root));
    game.root->addChild(main_behaviour);

    handlerFunc_t* init_handle = main_behaviour->getTrigger(game.event_init);
    if (init_handle)
    {
        typedef void (*event_init_function)(std::shared_ptr<CBehaviour>, Game*);
        reinterpret_cast<event_init_function>(init_handle)(main_behaviour, &game);
    }

    // Start render loop
    std::thread physicsThread(game_logic, &game, main_behaviour);

    while (game.isRunning.load())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
            case SDL_QUIT:
                std::cout << "Exiting" << std::endl;
                game.isRunning.store(false);
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    game.device->WaitIdle();

                    SDL_GetWindowSize(game.window, &game.windowWidth, &game.windowHeight);

                    // Call resize events
                    resize(game, main_behaviour);
                }
                break;
            }
        }

        // Trigger render_tick event
        // TODO: Apply to every behaviour on the tree
        handlerFunc_t* handle = main_behaviour->getTrigger(game.event_render_tick);
        if (handle)
        {
            typedef void (*event_init_function)(std::shared_ptr<CBehaviour>, Game*);
            reinterpret_cast<event_init_function>(handle)(main_behaviour, &game);
        }
    }

    // sync & exit
    physicsThread.join();

    shutdown(game);

    return 0;
}