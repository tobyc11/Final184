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

static void InspectorDFSNode(CSceneNode* scnNode, CSceneNode*& selectedNode)
{
    if (ImGui::TreeNode(scnNode->GetName().c_str()))
    {
        if (ImGui::IsItemClicked())
            selectedNode = scnNode;

        if (!scnNode->GetPrimitives().empty())
        {
            ImGui::Text("Primitives: %lu", scnNode->GetPrimitives().size());
            ImGui::Text("World bounds:");
            auto bb = scnNode->GetWorldBoundingBox();
            ImGui::DragFloat3("Min", &bb.Min.x);
            ImGui::DragFloat3("Max", &bb.Max.x);
        }
        auto t = scnNode->GetPosition();
        auto r = scnNode->GetRotation().EulerAngles();
        auto s = scnNode->GetScale();
        if (ImGui::DragFloat3("Translate", &t.x, 0.1f))
            scnNode->SetPosition(t);
        if (ImGui::DragFloat3("Rotate", &r.x, 0.1f))
        {
            tc::Quaternion quat;
            quat.FromEulerAngles(r.x, r.y, r.z);
            scnNode->SetRotation(quat);
        }
        if (ImGui::DragFloat3("Scale", &s.x, 0.1f))
            scnNode->SetScale(s.x);

        for (CSceneNode* childNode : scnNode->GetChildren())
            InspectorDFSNode(childNode, selectedNode);
        ImGui::TreePop();
    }

    if (selectedNode == scnNode)
    {
        for (const auto& prim : scnNode->GetPrimitives())
        {
            ImGui::Begin("Property Editor");
            prim->GetMaterial()->ImGuiEditor();
            ImGui::End();
        }
        for (const auto& light : scnNode->GetLights())
        {
            ImGui::Begin("Property Editor");
            light->ImGuiEditor();
            ImGui::End();
        }
    }
}

void CScene::ShowInspectorImGui()
{
    ImGui::Begin("Property Editor");
    ImGui::End();

    ImGui::Begin("Scene Inspector");
    if (ImGui::CollapsingHeader("Scene Tree"))
    {
        InspectorDFSNode(GetRootNode(), InspectorSelectedNode);
        ImGui::Separator();
    }
    ImGui::End();
}

} /* namespace Foreground */
