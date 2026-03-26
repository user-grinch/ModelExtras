#include "imgui.h"
#include "rwplcore.h"

extern bool gbGlobalIndicatorLights;
extern float gfGlobalCoronaSize;
extern int gGlobalCoronaIntensity;
extern int gGlobalShadowIntensity;

extern RwSurfaceProperties &gLightSurfProps;

static void HelpMarker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void Tab_Lights()
{
    if (ImGui::BeginTabItem("Settings"))
    {
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "General Configuration");
        ImGui::Separator();
        ImGui::Checkbox("Global Indicator Lights", &gbGlobalIndicatorLights);
        ImGui::SameLine();
        HelpMarker("Toggles indicators for all traffic vehicles globally.");

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Lighting Defaults");
        ImGui::Separator();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);

        ImGui::SliderInt("Corona Intensity", &gGlobalCoronaIntensity, 0, 255);
        ImGui::SliderFloat("Corona Size", &gfGlobalCoronaSize, 0.0f, 10.0f, "%.2f");
        ImGui::SliderInt("Shadow Intensity", &gGlobalShadowIntensity, 0, 255);

        ImGui::Spacing();
        ImGui::Text("Material Properties");
        ImGui::SliderFloat3("Ambient Color", (float *)&gLightSurfProps.ambient, 0.0f, 20.0f);

        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }
}
