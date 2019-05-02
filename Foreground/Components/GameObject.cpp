#include "GameObject.h"

#include <imgui.h>

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

void CGameObject::addChild(std::shared_ptr<CGameObject> obj) {
    children.push_back(obj);
}

std::shared_ptr<CGameObject> CGameObject::getFirstWithName(std::string name)
{
    if (this->toString().compare(name) == 0)
    {
        return shared_from_this();
    }

    for (auto node : children)
    {
        auto result = node->getFirstWithName(name);
        if (result)
            return result;
    }

    return nullptr;
}

static void InspectorDFSNode(CGameObject* node)
{
    if (ImGui::TreeNode(node->toString().c_str()))
    {
        for (auto child : node->children)
            InspectorDFSNode(child.get());
        ImGui::TreePop();
    }
}

void CGameObject::ShowInspectorImGui(CGameObject* node)
{
    ImGui::Begin("Game Graph Inspector");
    if (ImGui::CollapsingHeader("GameObject Tree"))
    {
        InspectorDFSNode(node);
        ImGui::Separator();
    }
    ImGui::End();
}

CEmptyGameObject::CEmptyGameObject(std::string name) { this->name = name; }

std::string CEmptyGameObject::toString() const { return name; }

}