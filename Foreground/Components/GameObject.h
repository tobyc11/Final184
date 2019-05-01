#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Foreground
{

    class CGameObject
    {
    public:
        std::shared_ptr<CGameObject> parent;
        std::vector<std::shared_ptr<CGameObject>> children;

        virtual std::string toString() const { return ""; }
        virtual size_t getInstanceID() const { return (size_t) this; }

        bool hasChild(std::shared_ptr<CGameObject> obj) const;
    };

}