#include "Light.h"

#include <imgui.h>

namespace Foreground
{

CLight::CLight(tc::Color c, float i, ELightType type)
{
    Color = c;
    Intensity = i;
    Type = type;
}

void CLight::ImGuiEditor()
{
    if (ImGui::CollapsingHeader("Light"))
    {
        ImGui::ColorEdit3("Color", &Color.r);
        ImGui::DragFloat("Intensity", &Intensity);
    }
}

}
