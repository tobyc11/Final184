#include "Event.h"
#include "Behaviour.h"


#include <iostream>

using namespace std;

namespace Foreground
{
    bool EventHandler::agreeWith(EventHandler* reference) {
        return reference->retType == retType
            && argType == reference->argType;
    }

    CEvent::CEvent(std::string name, EventHandler* handlerType) {
        this->name = name;
        referenceHandler = handlerType;
    }

    std::string CEvent::toString() const { return name; }

    size_t CEvent::getInstanceID() const { return std::hash<std::string> {}(name); }

}