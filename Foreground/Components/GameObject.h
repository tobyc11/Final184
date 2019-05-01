#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Foreground
{

class CGameObject : public std::enable_shared_from_this<CGameObject>
{
public:
    std::shared_ptr<CGameObject> parent;
    std::vector<std::shared_ptr<CGameObject>> children;

    virtual std::string toString() const { return ""; }
    virtual size_t getInstanceID() const { return (size_t)this; }

    bool hasChild(std::shared_ptr<CGameObject> obj) const;
    void addChild(std::shared_ptr<CGameObject> obj);

    std::shared_ptr<CGameObject> getFirstWithName(std::string name);
};

class CEmptyGameObject : public CGameObject
{
private:
    std::string name;

public:
    CEmptyGameObject(std::string name);

    std::string toString() const;
};

}