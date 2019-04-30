#include "ResourceManager.h"

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
//  Mesh stuff
// ============================================================================

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
};

struct ShadersUniform
{
    glm::vec4 color;
    glm::mat4 projection;
    glm::mat4 modelview;
    glm::mat4 projectionInverse;
    float camera_near;
    float camera_far;
};

struct Mesh
{
    CBuffer::Ref vbo;
    std::vector<Vertex> vertices;
};

struct Model
{
    std::vector<Mesh> meshes;
    std::vector<std::unique_ptr<Model>> subNodes;

    void render(IRenderContext::Ref ctx)
    {
        for (Mesh& m : meshes)
        {
            ctx->BindVertexBuffer(0, *m.vbo, 0);
            ctx->Draw(m.vertices.size(), 1, 0, 0);
        }

        for (const auto& m : subNodes)
        {
            m->render(ctx);
        }
    }
};

std::unique_ptr<Model> processNode(aiNode* node, const aiScene* scene, CDevice::Ref device)
{
    auto model = std::make_unique<Model>();

    if (node->mNumMeshes > 0)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh m;

            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                {
                    int ind = face.mIndices[j];
                    Vertex v = {
                        { mesh->mVertices[ind].x, mesh->mVertices[ind].y, mesh->mVertices[ind].z },
                        { mesh->mNormals[ind].x, mesh->mNormals[ind].y, mesh->mNormals[ind].z },
                        { 0.0, 0.0 },
                        { 1.0, 1.0, 1.0 }
                    };
                    if (mesh->mTextureCoords[0])
                    {
                        v.uv = glm::vec2(mesh->mTextureCoords[0][ind].x,
                                         mesh->mTextureCoords[0][ind].y);
                    }
                    m.vertices.push_back(v);
                }
            }

            m.vbo = device->CreateBuffer(m.vertices.size() * sizeof(Vertex),
                                         EBufferUsageFlags::VertexBuffer, m.vertices.data());

            model->meshes.push_back(m);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        model->subNodes.push_back(processNode(node->mChildren[i], scene, device));
    }

    return model;
}

std::unique_ptr<Model> loadModel(const std::string& path, CDevice::Ref device)
{
    Assimp::Importer import;
    const aiScene* scene =
        import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return nullptr;
    }
    // directory = path.substr(0, path.find_last_of('/'));

    return processNode(scene->mRootNode, scene, device);
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

void game_logic(CPipeline::Ref pso, CBuffer::Ref ubo)
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

        // Graphics view stuff
        ShadersUniform* uniform = static_cast<ShadersUniform*>(ubo->Map(0, sizeof(ShadersUniform)));
        uniform->color = { std::sin(elapsedTime), std::cos(elapsedTime), 1.0f, 1.0f };
        uniform->modelview = glm::lookAt(
            glm::vec3(std::sin(elapsedTime) * 16.0f, 8.0f, std::cos(elapsedTime) * 16.0f),
            glm::vec3(0.0f, 8.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo->Unmap();

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

    // Setup pipeline
    CPipeline::Ref pso_gbuffer;
    {
        CPipelineDesc pipelineDesc;
        pipelineDesc.VS = LoadSPIRV(device, "gbuffers.vert.spv");
        pipelineDesc.PS = LoadSPIRV(device, "gbuffers.frag.spv");
        pipelineDesc.RasterizerState.CullMode = ECullModeFlags::None;
        pipelineDesc.RenderPass = gbufferPass;

        CVertexInputBindingDesc vertInputBinding = { 0, sizeof(Vertex), false };

        pipelineDesc.VertexAttributes = {
            { 0, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, pos), 0 },
            { 1, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, normal), 0 },
            { 2, EFormat::R32G32_SFLOAT, offsetof(Vertex, uv), 0 },
            { 3, EFormat::R32G32B32_SFLOAT, offsetof(Vertex, color), 0 }
        };
        pipelineDesc.VertexBindings = { vertInputBinding };

        pso_gbuffer = device->CreatePipeline(pipelineDesc);
    }

    CPipeline::Ref pso_screen;
    {
        CPipelineDesc pipelineDesc;
        pipelineDesc.VS = LoadSPIRV(device, "deferred.vert.spv");
        pipelineDesc.PS = LoadSPIRV(device, "deferred.frag.spv");
        pipelineDesc.RasterizerState.CullMode = ECullModeFlags::None;
        pipelineDesc.RenderPass = screenPass;

        pso_screen = device->CreatePipeline(pipelineDesc);
    }

    // Setup uniforms
    auto ubo = device->CreateBuffer(sizeof(ShadersUniform), EBufferUsageFlags::ConstantBuffer);
    ShadersUniform* uniform = static_cast<ShadersUniform*>(ubo->Map(0, sizeof(ShadersUniform)));
    uniform->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    uniform->projection = glm::perspective(glm::radians(70.0), 640.0 / 480.0, 0.1, 50.0);
    uniform->projection[1][1] *= -1;
    uniform->projectionInverse = glm::inverse(uniform->projection);
    uniform->modelview = glm::mat4(1.0);
    ubo->Unmap();

    // Load model
    auto scene = loadModel(CResourceManager::Get().FindFile("nano.fbx"), device);

    // Setup texture
    CSamplerDesc samplerDesc;
    auto sampler = device->CreateSampler(samplerDesc);

    // Main render loop
    auto ctx = device->GetImmediateContext();

    std::thread physicsThread(game_logic, pso_gbuffer, ubo);

    while (!terminated)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                terminated = true;
        }

        // Draw GUI stuff
        CRHIImGuiBackend::NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::Begin("Controls");

        std::ostringstream oss;

        ImGui::Text("Physics tick rate: %f", physicsTickRate);
        ImGui::Text("Elapsed time: %f", elapsedTime);

        ImGui::End();

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
        ctx->BeginRenderPass(*gbufferPass,
                             { CClearValue(0.2f, 0.3f, 0.4f, 0.0f),
                               CClearValue(0.0f, 0.0f, 0.0f, 0.0f), CClearValue(1.0f, 0) });
        ctx->BindPipeline(*pso_gbuffer);
        ctx->BindBuffer(*ubo, 0, sizeof(ShadersUniform), 0, 1, 0);
        scene->render(ctx);
        ctx->EndRenderPass();

        ctx->BeginRenderPass(*screenPass, { CClearValue(0.0f, 0.0f, 0.0f, 0.0f) });
        ctx->BindPipeline(*pso_screen);
        ctx->BindBuffer(*ubo, 0, sizeof(ShadersUniform), 1, 0, 0);
        ctx->BindSampler(*sampler, 0, 0, 0);
        ctx->BindImageView(*gbuffer.albedoView, 0, 1, 0);
        ctx->BindImageView(*gbuffer.normalsView, 0, 2, 0);
        ctx->BindImageView(*gbuffer.depthView, 0, 3, 0);
        ctx->Draw(6, 1, 0, 0);

        ImGui::Render();
        CRHIImGuiBackend::RenderDrawData(ImGui::GetDrawData(), ctx);

        ctx->EndRenderPass();

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
