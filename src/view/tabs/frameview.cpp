#include "pch.h"
#include "imgui/imgui.h"
#include "view/common.hpp"

class VehicleFrameView
{
    static inline ImGuiTextFilter s_Filter;

    // Helper to check if any children match the filter (so we know to keep parents open)
    static bool AnyDescendantMatches(RwFrame* frame)
    {
        // If filter is NOT active (empty), everything matches
        if (!s_Filter.IsActive()) return true;

        while (frame)
        {
            const char* name = GetFrameNodeName(frame);
            if (s_Filter.PassFilter(name))
                return true;

            if (frame->child && AnyDescendantMatches(frame->child))
                return true;

            frame = frame->next;
        }
        return false;
    }

    static void RenderNode(RwFrame* frame)
    {
        while (frame)
        {
            const char* name = GetFrameNodeName(frame);
            bool hasChildren = (frame->child != nullptr);

            // --- SEARCH FILTER LOGIC ---
            bool nodeMatches = s_Filter.PassFilter(name);
            bool childrenMatch = hasChildren && AnyDescendantMatches(frame->child);

            // If filter IS active:
            if (s_Filter.IsActive())
            {
                // If neither this node nor its children match, skip rendering
                if (!nodeMatches && !childrenMatch)
                {
                    frame = frame->next;
                    continue;
                }

                // If children match, auto-expand this node
                if (childrenMatch)
                    ImGui::SetNextItemOpen(true);
            }
            // ---------------------------

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren)
                flags |= ImGuiTreeNodeFlags_Leaf;

            // Highlight text if it matches filter
            if (s_Filter.IsActive() && nodeMatches)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));

            bool isNodeOpen = ImGui::TreeNodeEx(frame, flags, "%s", name);

            if (s_Filter.IsActive() && nodeMatches)
                ImGui::PopStyleColor();

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Copy Name"))
                    ImGui::SetClipboardText(name);

                ImGui::TextDisabled("Ptr: %p", frame);
                ImGui::EndPopup();
            }

            if (isNodeOpen)
            {
                if (hasChildren)
                    RenderNode(frame->child);

                ImGui::TreePop();

                // --- VISUAL SEPARATION ---
                // Add separator only if we just finished rendering children
                if (hasChildren)
                {
                    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                }
            }

            frame = frame->next;
        }
    }

public:
    static void Render(CVehicle* vehicle)
    {
        if (!vehicle || !vehicle->m_pRwClump)
            return;

        RwFrame* rootFrame = (RwFrame*)vehicle->m_pRwClump->object.parent;

        // Search Bar
        ImGui::SetNextItemWidth(-1);
        s_Filter.Draw("##SearchFrame", -FLT_MIN);

        ImGui::Separator();

        if (ImGui::BeginChild("FrameTreeScrolling", ImVec2(0, 0), true))
        {
            RenderNode(rootFrame);
        }
        ImGui::EndChild();
    }
};

void Tab_FrameView() {
    if (ImGui::BeginTabItem("FrameView")) {
        CVehicle* pVeh = FindPlayerVehicle();
        if (pVeh) {
            VehicleFrameView::Render(pVeh);
        } else {
            DrawVehicleRequiredMessage();
        }
        ImGui::EndTabItem();
    }
}