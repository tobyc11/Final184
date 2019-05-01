#include "Event.h"

namespace Foreground
{
    bool EventHandler::agreeWith(EventHandler& reference) {
        return reference.retType == retType
            && std::equal(argType.begin(), argType.end(), reference.argType.begin());
    }

    CEvent::CEvent(std::string name) { this->name = name; }

    std::string CEvent::toString() const { return ""; }

    size_t CEvent::getInstanceID() const { return std::hash<std::string> {}(name); }
}