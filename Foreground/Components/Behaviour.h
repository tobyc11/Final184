#pragma once

#include "GameObject.h"
#include "Event.h"

#include <unordered_map>

namespace Foreground
{

    class CBehaviour : public CGameObject
    {
    public:
        std::unordered_map<std::shared_ptr<CEvent>, EventHandler*> handles;

        handlerFunc_t* getTrigger(std::shared_ptr<CEvent> event) const;
        bool registerFirstNamedEvent(CGameObject* root, std::string name, EventHandler*);
    };

}