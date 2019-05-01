#include "GameObject.h"

namespace Foreground
{

    bool CGameObject::hasChild(std::shared_ptr<CGameObject> obj) const
    {
        for (auto child : children)
        {
            if (child == obj)
            {
                return true;   
            }
        }
        return false;
    }

}