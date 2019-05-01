#pragma once

// Engine related stuff
#include <Resources/ResourceManager.h>

#include <PresentationSurfaceDesc.h>
#include <RHIImGuiBackend.h>
#include <RHIInstance.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include <Components/Behaviour.h>
#include <Components/Event.h>
#include <Components/GameObject.h>
#include <ForegroundBootstrapper.h>

// Platform support
#include <Platform.h>

#if TC_OS == TC_OS_MAC_OS_X
#include <AbstractionBreaker.h>
#include <SDL_vulkan.h>
#endif

#if TC_OS == TC_OS_WINDOWS_NT
#include <Windows.h>
#elif TC_OS == TC_OS_LINUX
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#undef None
#endif

// imgui stuff
#include "imgui.h"
#include "imgui_impl_sdl.h"

// cpp STL templates
#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>

using namespace Foreground;

using namespace RHI;

// ----------------------------------------------------------------------------
// Global structures
// ----------------------------------------------------------------------------

struct Game
{
    SDL_Window* window;
    CPresentationSurfaceDesc surfaceDesc;
    CDevice::Ref device;
    CSwapChain::Ref swapChain;
    std::atomic<bool> isRunning;

    CGameObject* root;

    std::shared_ptr<CEvent> event_init;
    std::shared_ptr<CEvent> event_logic_tick;
    std::shared_ptr<CEvent> event_render_tick;
};