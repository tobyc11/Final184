#pragma once

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

class Control {
public:
    enum class Sticky {
        MoveLeft = 0,
        MoveRight,
        MoveForward,
        MoveBack,
        MoveUp,
        MoveDown,
        MAX_VAL
    };
    enum class Analog {
        CameraPitch = 0, // Up/down
        CameraYaw, // Left/right
        MAX_VAL
    };

private:
    std::atomic_bool sticky_states[(size_t)Sticky::MAX_VAL] = {};
    std::atomic<float> analog_states[(size_t)Analog::MAX_VAL] = {};

public:
    void setSticky(Sticky which, bool active) {
        sticky_states[(size_t)which] = active;
    }
    bool getSticky(Sticky which) const {
        return sticky_states[(size_t)which];
    }

    void setAnalog(Analog which, float val) {
        analog_states[(size_t)which] = val;
    }
    void lerpAnalog(Analog which, float target, float t) {
        float old = getAnalog(which);
        float out = old * (1-t) + target * t;
        setAnalog(which, out);
    }
    float getAnalog(Analog which) const {
        return analog_states[(size_t)which];
    }

    int xAxis() const {
        return getSticky(Sticky::MoveLeft) - getSticky(Sticky::MoveRight);
    }
    int yAxis() const {
        return getSticky(Sticky::MoveUp) - getSticky(Sticky::MoveDown);
    }
    int zAxis() const {
        return getSticky(Sticky::MoveForward) - getSticky(Sticky::MoveBack);
    }
};

struct Game
{
    SDL_Window* window;
    CPresentationSurfaceDesc surfaceDesc;
    CDevice::Ref device;
    CSwapChain::Ref swapChain;
    std::atomic<bool> isRunning;

    CGameObject* root;

    int windowWidth, windowHeight;

    std::shared_ptr<CEvent> event_init;
    std::shared_ptr<CEvent> event_logic_tick;
    std::shared_ptr<CEvent> event_render_tick;
    std::shared_ptr<CEvent> event_resize;
    Control control;
};