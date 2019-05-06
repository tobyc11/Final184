#include "Behaviour.h"

#include <cassert>

namespace Foreground
{

handlerFunc_t* CBehaviour::getTrigger(std::shared_ptr<CEvent> event) const
{
    // TODO: Replace this find with contains after C++20 is adopted
    if (handles.find(event) != handles.end())
    {
        EventHandler* h = handles.at(event);

        assert(event->referenceHandler->agreeWith(h));

        return h->handle;
    }

    return nullptr;
}

bool CBehaviour::registerFirstNamedEvent(CGameObject* root, std::string name, EventHandler* handle)
{
    std::shared_ptr<CEvent> ev = std::dynamic_pointer_cast<CEvent>(root->getFirstWithName(name));
    if (ev)
    {
        handles.insert_or_assign(ev, handle);
        return true;
    }
    return false;
}

}
