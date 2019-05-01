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

}

