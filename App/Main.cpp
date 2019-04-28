#include <Resources/ResourceManager.h>

#include <Platform.h>

#if TC_OS == TC_OS_MAC_OS_X
// Abstraction breaker will pull in vulkan.h, which is used by presentation surface desc
#include <AbstractionBreaker.h>
#include <SDL_vulkan.h>
#endif

#include <PresentationSurfaceDesc.h>
#include <RHIImGuiBackend.h>
#include <RHIInstance.h>
#include <SDL.h>
#include <SDL_syswm.h>

#if TC_OS == TC_OS_WINDOWS_NT
#include <Windows.h>
#elif TC_OS == TC_OS_LINUX
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#undef None
#endif

#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"

#include <SceneGraph/Scene.h>
#include <SceneGraph/glTFSceneImporter.h>

#include <Renderer/RenderConductor.h>
#include <Renderer/DeferredConductor.h>

using namespace Foreground;

using namespace RHI;

// ============================================================================
//  LoadSPIRV : Load compiled SPRIV shaders
// ============================================================================
static CShaderModule::Ref LoadSPIRV(CDevice::Ref device, const std::string& name)
{
    std::ifstream file(CResourceManager::Get().FindShader(name).c_str(),
                       std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
        return device->CreateShaderModule(buffer.size(), buffer.data());
    return {};
}

// ============================================================================
//  CreateScreenPass : Create Scene Render Pass
// ============================================================================
struct GBuffer
{
    CImageView::Ref albedoView;
    CImageView::Ref normalsView;
    CImageView::Ref depthView;
};

static CRenderPass::Ref CreateGBufferPass(CDevice::Ref device, CSwapChain::Ref swapChain,
                                          GBuffer& gbuffer)
{
    /*auto fbImage = swapChain->GetImage();*/
    uint32_t width, height;
    swapChain->GetSize(width, height);

    auto albedoImage = device->CreateImage2D(
        EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
        height);
    auto normalsImage = device->CreateImage2D(
        EFormat::R8G8B8A8_UNORM, EImageUsageFlags::RenderTarget | EImageUsageFlags::Sampled, width,
        height);
    auto depthImage = device->CreateImage2D(
        EFormat::D32_SFLOAT_S8_UINT, EImageUsageFlags::DepthStencil | EImageUsageFlags::Sampled,
        width, height);

    CImageViewDesc albedoViewDesc;
    albedoViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    albedoViewDesc.Type = EImageViewType::View2D;
    albedoViewDesc.Range.Set(0, 1, 0, 1);
    auto albedoView = device->CreateImageView(albedoViewDesc, albedoImage);
    gbuffer.albedoView = albedoView;

    CImageViewDesc normalsViewDesc;
    normalsViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    normalsViewDesc.Type = EImageViewType::View2D;
    normalsViewDesc.Range.Set(0, 1, 0, 1);
    auto normalsView = device->CreateImageView(normalsViewDesc, normalsImage);
    gbuffer.normalsView = normalsView;

    CImageViewDesc depthViewDesc;
    depthViewDesc.Format = EFormat::D32_SFLOAT_S8_UINT;
    depthViewDesc.Type = EImageViewType::View2D;
    depthViewDesc.DepthStencilAspect = EDepthStencilAspectFlags::Depth;
    depthViewDesc.Range.Set(0, 1, 0, 1);
    auto depthView = device->CreateImageView(depthViewDesc, depthImage);
    gbuffer.depthView = depthView;

    CRenderPassDesc rpDesc;
    rpDesc.AddAttachment(albedoView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.AddAttachment(normalsView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.AddAttachment(depthView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.NextSubpass().AddColorAttachment(0).AddColorAttachment(1).SetDepthStencilAttachment(2);
    swapChain->GetSize(rpDesc.Width, rpDesc.Height);
    rpDesc.Layers = 1;
    return device->CreateRenderPass(rpDesc);
}

static CRenderPass::Ref CreateScreenPass(CDevice::Ref device, CSwapChain::Ref swapChain)
{
    auto fbImage = swapChain->GetImage();
    uint32_t width, height;
    swapChain->GetSize(width, height);

    CImageViewDesc fbViewDesc;
    fbViewDesc.Format = EFormat::R8G8B8A8_UNORM;
    fbViewDesc.Type = EImageViewType::View2D;
    fbViewDesc.Range.Set(0, 1, 0, 1);
    auto fbView = device->CreateImageView(fbViewDesc, fbImage);

    CRenderPassDesc rpDesc;
    rpDesc.AddAttachment(fbView, EAttachmentLoadOp::Clear, EAttachmentStoreOp::Store);
    rpDesc.Subpasses.resize(1);
    rpDesc.Subpasses[0].AddColorAttachment(0);
    swapChain->GetSize(rpDesc.Width, rpDesc.Height);
    rpDesc.Layers = 1;
    return device->CreateRenderPass(rpDesc);
}

// ============================================================================
//  Physics & game logic thread
// ============================================================================
bool terminated = false;
double physicsTickRate = 0.0;
double elapsedTime = 0.0;

const int tickRate = 120;
const double tickInterval = 1.0 / (double)tickRate;

using namespace std::chrono_literals;

class Control {
public:
    enum class Sticky {
        MoveLeft = 0,
        MoveRight,
        MoveForward,
        MoveBack,
        MAX_VAL
    };
    enum class Analog {
        CameraPitch = 0, // Up/down
        CameraYaw, // Left/right
        MAX_VAL
    };

private:
    bool sticky_states[(size_t)Sticky::MAX_VAL] = {};
    float analog_states[(size_t)Analog::MAX_VAL] = {};

public:
    void setSticky(Sticky which, bool active) {
        sticky_states[(size_t)which] = active;
    }
    bool getSticky(Sticky which) {
        return sticky_states[(size_t)which];
    }

    void setAnalog(Analog which, float val) {
        analog_states[(size_t)which] = val;
    }

    int xAxis() {
        return getSticky(Sticky::MoveLeft) - getSticky(Sticky::MoveRight);
    }
    int yAxis() {
        return getSticky(Sticky::MoveForward) - getSticky(Sticky::MoveBack);
    }
};

void game_logic()
{
    uint64_t ticksElapsed = 0;
    uint64_t lastIntervalTicks = 0;

    auto timeStart = std::chrono::steady_clock::now();

    double tickRateLastUpdated = -1.0;

    while (!terminated)
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


        ticksElapsed++;

        std::this_thread::sleep_until(timeTarget);
    }
}

// ============================================================================
//  main
// ============================================================================
int main(int argc, char* argv[])
{
    CResourceManager::Get().Init();

    // Initialize devices & windows
    auto device = CInstance::Get().CreateDevice(EDeviceCreateHints::Discrete);
    SDL_Window* window;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("RHI Triangle Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              640, 480,
                              SDL_WINDOW_RESIZABLE
#if TC_OS == TC_OS_MAC_OS_X
                                  | SDL_WINDOW_VULKAN
#endif
    );

    if (window == nullptr)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);

    // Bind presentation surface to window
    CPresentationSurfaceDesc surfaceDesc;
#if TC_OS == TC_OS_WINDOWS_NT
    surfaceDesc.Win32.Instance = wmInfo.info.win.hinstance;
    surfaceDesc.Win32.Window = wmInfo.info.win.window;
    surfaceDesc.Type = EPresentationSurfaceDescType::Win32;
#elif TC_OS == TC_OS_LINUX
    surfaceDesc.Linux.xconn = XGetXCBConnection(wmInfo.info.x11.display);
    surfaceDesc.Linux.window = wmInfo.info.x11.window;
    surfaceDesc.Type = EPresentationSurfaceDescType::Linux;
#elif TC_OS == TC_OS_MAC_OS_X
    auto nativeDevice = RHI::GetNativeDevice(device);
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SDL_Vulkan_CreateSurface(window, nativeDevice.Instance, &surface);
    surfaceDesc.Type = EPresentationSurfaceDescType::Vulkan;
    surfaceDesc.Vulkan.Surface = surface;
#endif
    auto swapChain = device->CreateSwapChain(surfaceDesc, EFormat::R8G8B8A8_UNORM);

    GBuffer gbuffer;

    auto gbufferPass = CreateGBufferPass(device, swapChain, gbuffer);
    auto screenPass = CreateScreenPass(device, swapChain);

    // Setup ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForVulkan(window);
    CRHIImGuiBackend::Init(device, screenPass);

    // Load model
    std::shared_ptr<CScene> scene = std::make_shared<CScene>();

    CglTFSceneImporter importer(scene, *device);
    importer.ImportFile(CResourceManager::Get().FindFile("Models/Sponza.gltf"));

    // Start render loop
    auto ctx = device->GetImmediateContext();

    std::thread physicsThread(game_logic);
    Control control;

    // Setup Render Conductor
    DeferredConductor conductor(ctx, device, swapChain);

    while (!terminated)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
            case SDL_QUIT:
                terminated = true;
                break;
            case SDL_WINDOWEVENT:
            case SDL_WINDOWEVENT_RESIZED:
            {
                swapChain->Resize(UINT32_MAX, UINT32_MAX);
                uint32_t width, height;
                swapChain->GetSize(width, height);
                screenPass->Resize(width, height);
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                SDL_Keysym key = event.key.keysym;
                int state = event.key.state;

                Control::Sticky which_input;
                bool input_valid = !event.key.repeat;

                switch (key.scancode) {
                case SDL_SCANCODE_W:
                    which_input = Control::Sticky::MoveForward; break;
                case SDL_SCANCODE_A:
                    which_input = Control::Sticky::MoveLeft; break;
                case SDL_SCANCODE_S:
                    which_input = Control::Sticky::MoveBack; break;
                case SDL_SCANCODE_D:
                    which_input = Control::Sticky::MoveRight; break;
                case SDL_SCANCODE_ESCAPE:
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    break;
                case SDL_SCANCODE_RETURN:
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    break;
                default:
                    input_valid = false; break;
                }

                if (!input_valid) break;

                control.setSticky(which_input, state == SDL_PRESSED);
                break;
            }
            case SDL_MOUSEMOTION:
            {
                control.setAnalog(Control::Analog::CameraYaw, event.motion.xrel);
                control.setAnalog(Control::Analog::CameraPitch, event.motion.yrel);
                break;
            }
            }
        }

        // Draw GUI stuff
        /*CRHIImGuiBackend::NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::Begin("Controls");

        std::ostringstream oss;

        ImGui::Text("Physics tick rate: %f", physicsTickRate);
        ImGui::Text("Elapsed time: %f", elapsedTime);

        ImGui::End();*/

        // swapchain stuff
        bool swapOk = swapChain->AcquireNextImage();
        if (!swapOk)
        {
            swapChain->AutoResize();
            uint32_t width, height;
            swapChain->GetSize(width, height);
            screenPass->Resize(width, height);
            gbufferPass->Resize(width, height);
            swapChain->AcquireNextImage();
        }

        // Record render pass
        conductor.ProcessAndSubmit();

        // Present
        CSwapChainPresentInfo info;
        info.SrcImage = nullptr;
        swapChain->Present(info);
    }

    physicsThread.join();

    // Sync & exit
    ctx->Flush(true);
    CRHIImGuiBackend::Shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    CResourceManager::Get().Shutdown();
    return 0;
}
