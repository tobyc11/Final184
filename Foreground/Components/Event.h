#pragma once

#include "GameObject.h"

#include <typeindex>

#define VOID_TYPE std::type_index(typeid(void))

namespace Foreground
{

    typedef void (*handlerFunc_t)(void);

    struct EventHandler
    {
        std::type_index retType;
        std::vector<std::type_index> argType;
        
        handlerFunc_t* handle;

        bool agreeWith(EventHandler* reference);
    };

    class CEvent : public CGameObject
    {
    private:
        std::string name;

    public:
        EventHandler* referenceHandler;
        
        CEvent(std::string name, EventHandler* handlerType);

        std::string toString() const;
        size_t getInstanceID() const;
    };

}