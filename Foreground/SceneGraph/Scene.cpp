#include "Scene.h"
#include <imgui.h>

namespace Foreground
{

CScene::CScene()
{
    AccelStructure = std::make_unique<COctree>(100.0f);
    RootNode = std::make_unique<CSceneNode>(this, nullptr);
    RootNode->SetName("Root");
}

CSceneNode* CScene::GetRootNode() const { return RootNode.get(); }

COctree* CScene::GetAccelStructure() const { return AccelStructure.get(); }

static void UpdateAccelStructureDFS(CSceneNode* scnNode)
{
    scnNode->UpdateAccelStructure();
    for (CSceneNode* childNode : scnNode->GetChildren())
        UpdateAccelStructureDFS(childNode);
}

void CScene::UpdateAccelStructure() const { UpdateAccelStructureDFS(GetRootNode()); }

static void InspectorDFSNode(CSceneNode* scnNode)
{
    if (ImGui::TreeNode(scnNode->GetName().c_str()))
    {
        if (!scnNode->GetPrimitives().empty())
        {
            ImGui::Text("Primitives: %lu", scnNode->GetPrimitives().size());
            ImGui::Text("World bounds:");
            auto bb = scnNode->GetWorldBoundingBox();
            ImGui::DragFloat3("Min", &bb.Min.x);
            ImGui::DragFloat3("Max", &bb.Max.x);
        }
        auto t = scnNode->GetPosition();
        auto r = scnNode->GetRotation();
        auto s = scnNode->GetScale();
        ImGui::DragFloat3("Translate", &t.x, 0.1f);
        ImGui::DragFloat4("Rotate", &r.w, 0.1f);
        ImGui::DragFloat3("Scale", &s.x, 0.1f);

        for (CSceneNode* childNode : scnNode->GetChildren())
            InspectorDFSNode(childNode);
        ImGui::TreePop();
    }
}

void CScene::ShowInspectorImGui()
{
    ImGui::Begin("Scene Inspector");
    if (ImGui::CollapsingHeader("Scene Tree"))
    {
        InspectorDFSNode(GetRootNode());
        ImGui::Separator();
    }
    ImGui::End();
}

} /* namespace Foreground */
