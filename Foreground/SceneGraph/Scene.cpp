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

static void InspectorDFSNode(CSceneNode* scnNode)
{
    if (ImGui::TreeNode(scnNode->GetName().c_str()))
    {
        if (!scnNode->GetPrimitives().empty())
            ImGui::Text("Primitives: %lu", scnNode->GetPrimitives().size());

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
