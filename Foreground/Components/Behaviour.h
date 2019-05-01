#pragma once

#include "GameObject.h"

#include <unordered_map>

namespace Foreground
{

    class CBehaviour : public CGameObject
    {
    public:
        std::unordered_map<std::shared_ptr<CEvent>, EventHandler*> handles;
    };

}