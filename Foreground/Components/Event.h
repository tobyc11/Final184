#pragma once

#include "GameObject.h"

#include <typeindex>

namespace Foreground
{

    typedef void (*handlerFunc_t)(void);

    struct EventHandler
    {
        std::type_index retType;
        std::vector<std::type_index> argType;
        
        handlerFunc_t* handle;

        bool agreeWith(EventHandler& reference);
    };

    class CEvent : public CGameObject
    {
    private:
        std::string name;

    public:
        CEvent(std::string name);

        std::string toString() const;
        size_t getInstanceID() const;
    };

}