#include "pch.h"
#include "defines.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h" // Sometimes needed for specific flags
#include "CVehicle.h"
#include "CWorld.h"
#include "view/common.hpp"
#include <string>
#include <vector>

// Assumed Headers (Based on your context)
// #include "FeatureMgr.h"
// #include "magic_enum.hpp"
// #include "ModelExtrasEditor.h"

// =================================================================================
// 1. SAFE PROXY MACROS
// Solves "Address of bit-field" errors and prevents stack corruption.
// =================================================================================

// Helper to filter flags text
static ImGuiTextFilter FlagFilter;

// Boolean Flag (Bit-field safe)
#define BF_BOOL(name, var) { \
    bool b_temp = (var); \
    if (ImGui::Checkbox(name, &b_temp)) { var = b_temp; } \
}

// Searchable Boolean Flag
#define BF_BOOL_SEARCH(name, var) \
    if (FlagFilter.PassFilter(name)) { \
        bool b_temp = (var); \
        if (ImGui::Checkbox(name, &b_temp)) { var = b_temp; } \
    }

// Multi-Bit Field (e.g. unsigned char : 3)
#define BF_MBIT(name, var, max_val) { \
    int i_temp = (int)(var); \
    if (ImGui::SliderInt(name, &i_temp, 0, max_val)) { var = (decltype(var))i_temp; } \
}

// General Scalar Field (Byte, Short, Enum) - Prevents size mismatch writing
#define VAL_FIELD(name, var) { \
    int i_temp = (int)(var); \
    if (ImGui::InputInt(name, &i_temp)) { var = (decltype(var))i_temp; } \
}

// Read-only Pointer Visualizer
#define DRAW_PTR(label, ptr) { \
    ImGui::TextDisabled("%s: ", label); \
    ImGui::SameLine(); \
    if (ptr) ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "0x%p", ptr); \
    else ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "NULL"); \
}

class VehicleFrameView
{
    static void RenderNode(RwFrame* frame)
    {
        while (frame)
        {
            const char* name = GetFrameNodeName(frame); // Assuming this helper exists
            bool hasChildren = (frame->child != nullptr);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

            bool isNodeOpen = ImGui::TreeNodeEx(frame, flags, "%s", name);

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Copy Name")) ImGui::SetClipboardText(name);
                ImGui::TextDisabled("Ptr: %p", frame);
                ImGui::EndPopup();
            }

            if (isNodeOpen)
            {
                if (hasChildren) RenderNode(frame->child);
                ImGui::TreePop();
            }

            frame = frame->next;
        }
    }

public:
    static void Render(CVehicle* vehicle)
    {
        if (!vehicle || !vehicle->m_pRwClump) {
            ImGui::Text("Invalid RwClump");
            return;
        }

        RwFrame* rootFrame = (RwFrame*)vehicle->m_pRwClump->object.parent;

        if (ImGui::BeginChild("FrameTreeScrolling", ImVec2(0, 0), true))
        {
            RenderNode(rootFrame);
        }
        ImGui::EndChild();
    }
};

// =================================================================================
// 4. MONSTER INSPECTOR (DETAILED ANALYSIS)
// =================================================================================

void DrawMonsterInspector()
{
    CVehicle* pVeh = FindPlayerVehicle(0, false);

    if (!pVeh) {
        DrawVehicleRequiredMessage();
        return;
    }

    // --- DASHBOARD HEADER ---
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    if (ImGui::BeginChild("Dashboard", ImVec2(0, 90), true)) {
        ImGui::Columns(3, "DashCols", false);

        // Col 1: ID
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Model: %d", pVeh->m_nModelIndex);
        ImGui::TextDisabled("Ptr: 0x%p", pVeh);
        VAL_FIELD("Class", pVeh->m_nVehicleClass);

        ImGui::NextColumn();

        // Col 2: Status
        float velocity = pVeh->m_vecMoveSpeed.Magnitude() * 180.0f;
        ImGui::Text("Speed: %.1f km/h", velocity);

        bool bEng = pVeh->bEngineOn;
        ImGui::Checkbox("Engine", &bEng); pVeh->bEngineOn = bEng;
        ImGui::SameLine();
        bool bLgt = pVeh->bLightsOn;
        ImGui::Checkbox("Lights", &bLgt); pVeh->bLightsOn = bLgt;

        ImGui::NextColumn();

        // Col 3: Health
        float healthPct = pVeh->m_fHealth / 1000.0f;
        if (healthPct < 0) healthPct = 0; if (healthPct > 1) healthPct = 1;
        ImVec4 hColor = (healthPct > 0.5f) ? ImVec4(0,1,0,1) : (healthPct > 0.25f) ? ImVec4(1,1,0,1) : ImVec4(1,0,0,1);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hColor);
        char hpOverlay[32]; sprintf(hpOverlay, "%.0f HP", pVeh->m_fHealth);
        ImGui::ProgressBar(healthPct, ImVec2(-1, 0), hpOverlay);
        ImGui::PopStyleColor();

        VAL_FIELD("Gear", pVeh->m_nCurrentGear);

        ImGui::Columns(1);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- TABS ---
    if (ImGui::BeginTabBar("MonsterTabs"))
    {
        // TAB 1: HIERARCHY
        if (ImGui::BeginTabItem("Hierarchy"))
        {
            if (ImGui::CollapsingHeader("CPlaceable", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (pVeh->m_matrix) {
                    ImGui::DragFloat3("Matrix Pos", (float*)&pVeh->m_matrix->pos, 0.1f);
                    DRAW_PTR("Matrix", pVeh->m_matrix);
                } else {
                    ImGui::DragFloat3("Placement Pos", (float*)&pVeh->m_placement.m_vPosn, 0.1f);
                }
                ImGui::DragFloat("Heading", &pVeh->m_placement.m_fHeading, 0.01f);
            }

            if (ImGui::CollapsingHeader("CEntity", ImGuiTreeNodeFlags_DefaultOpen)) {
                DRAW_PTR("RW Object", pVeh->m_pRwObject);
                VAL_FIELD("Type (0-7)", pVeh->m_nType);
                VAL_FIELD("Status (0-31)", pVeh->m_nStatus);

                ImGui::Separator();
                if (ImGui::TreeNode("Entity Flags")) {
                    BF_BOOL("bUsesCollision", pVeh->bUsesCollision);
                    BF_BOOL("bCollisionProcessed", pVeh->bCollisionProcessed);
                    BF_BOOL("bIsVisible", pVeh->bIsVisible);
                    BF_BOOL("bRenderDamaged", pVeh->bRenderDamaged);
                    BF_BOOL("bIsStatic", pVeh->bIsStatic);
                    BF_BOOL("bBackfaceCulled", pVeh->bBackfaceCulled);
                    ImGui::TreePop();
                }
            }

            if (ImGui::CollapsingHeader("CPhysical", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Mass: %.1f | Turn Mass: %.1f", pVeh->m_fMass, pVeh->m_fTurnMass);
                ImGui::DragFloat3("Move Speed", (float*)&pVeh->m_vecMoveSpeed, 0.001f);
                ImGui::DragFloat3("Turn Speed", (float*)&pVeh->m_vecTurnSpeed, 0.001f);

                if (ImGui::TreeNode("Immunities & Physics")) {
                    BF_BOOL("bBulletProof", pVeh->bBulletProof);
                    BF_BOOL("bFireProof", pVeh->bFireProof);
                    BF_BOOL("bExplosionProof", pVeh->bExplosionProof);
                    BF_BOOL("bCollisionProof", pVeh->bCollisionProof);
                    BF_BOOL("bMeleeProof", pVeh->bMeleeProof);
                    ImGui::Separator();
                    BF_BOOL("bApplyGravity", pVeh->bApplyGravity);
                    BF_BOOL("bCollidable", pVeh->bCollidable);
                    ImGui::TreePop();
                }
            }
            ImGui::EndTabItem();
        }

        // TAB 2: SEARCHABLE FLAGS
        if (ImGui::BeginTabItem("Flags Search"))
        {
            ImGui::Spacing();
            FlagFilter.Draw("##flagfilter", -1.0f);
            if (!FlagFilter.IsActive()) ImGui::TextDisabled("Search flags (e.g. 'siren', 'lock')");
            ImGui::Separator();

            if (ImGui::BeginChild("FlagsScroll")) {
                BF_BOOL_SEARCH("bIsLawEnforcer", pVeh->bIsLawEnforcer);
                BF_BOOL_SEARCH("bIsAmbulanceOnDuty", pVeh->bIsAmbulanceOnDuty);
                BF_BOOL_SEARCH("bIsFireTruckOnDuty", pVeh->bIsFireTruckOnDuty);
                BF_BOOL_SEARCH("bIsLocked", pVeh->bIsLocked);
                BF_BOOL_SEARCH("bEngineOn", pVeh->bEngineOn);
                BF_BOOL_SEARCH("bIsHandbrakeOn", pVeh->bIsHandbrakeOn);
                BF_BOOL_SEARCH("bLightsOn", pVeh->bLightsOn);
                BF_BOOL_SEARCH("bSirenOrAlarm", pVeh->bSirenOrAlarm);
                BF_BOOL_SEARCH("bTakeLessDamage", pVeh->bTakeLessDamage);
                BF_BOOL_SEARCH("bIsDamaged", pVeh->bIsDamaged);
                BF_BOOL_SEARCH("bCanBeDamaged", pVeh->bCanBeDamaged);
                BF_BOOL_SEARCH("bTyresDontBurst", pVeh->bTyresDontBurst);
                BF_BOOL_SEARCH("bPetrolTankIsWeakPoint", pVeh->bPetrolTankIsWeakPoint);
                BF_BOOL_SEARCH("bCreatedAsPoliceVehicle", pVeh->bCreatedAsPoliceVehicle);
                BF_BOOL_SEARCH("bMadDriver", pVeh->bMadDriver);
                BF_BOOL_SEARCH("bUseCarCheats", pVeh->bUseCarCheats);
                BF_BOOL_SEARCH("bHasBeenResprayed", pVeh->bHasBeenResprayed);
                BF_BOOL_SEARCH("bIsVan", pVeh->bIsVan);
                BF_BOOL_SEARCH("bIsBus", pVeh->bIsBus);
                BF_BOOL_SEARCH("bIsBig", pVeh->bIsBig);
                BF_BOOL_SEARCH("bLowVehicle", pVeh->bLowVehicle);
                BF_BOOL_SEARCH("bIsRCVehicle", pVeh->bIsRCVehicle);
                // Add remaining 100+ flags here using BF_BOOL_SEARCH
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // TAB 3: DATA
        if (ImGui::BeginTabItem("Data"))
        {
            ImGui::Text("Creation Time: %u", pVeh->m_nCreationTime);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Controls");
            ImGui::SliderFloat("Steer", &pVeh->m_fSteerAngle, -1.0f, 1.0f);
            ImGui::SliderFloat("Gas", &pVeh->m_fGasPedal, 0.0f, 1.0f);
            ImGui::SliderFloat("Brake", &pVeh->m_fBreakPedal, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Appearance");
            int colors[4] = { pVeh->m_nPrimaryColor, pVeh->m_nSecondaryColor, pVeh->m_nTertiaryColor, pVeh->m_nQuaternaryColor };
            if (ImGui::InputInt4("Colors", colors)) {
                pVeh->m_nPrimaryColor = colors[0]; pVeh->m_nSecondaryColor = colors[1];
                pVeh->m_nTertiaryColor = colors[2]; pVeh->m_nQuaternaryColor = colors[3];
            }
            ImGui::SliderFloat("Dirt", &pVeh->m_fDirtLevel, 0.0f, 15.0f);

            if (ImGui::TreeNode("Upgrades")) {
                for (int i=0; i<15; i++) {
                     char label[16]; sprintf(label, "Slot %d", i);
                     VAL_FIELD(label, pVeh->m_anUpgrades[i]);
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("AutoPilot")) {
                ImGui::DragFloat3("Dest", (float*)&pVeh->m_autoPilot.m_vecDestinationCoors);
                VAL_FIELD("Mode", pVeh->m_autoPilot.m_nCarMission);
                VAL_FIELD("Cruise Speed", pVeh->m_autoPilot.m_nCruiseSpeed);
                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }

        // TAB 4: INTERNALS
        if (ImGui::BeginTabItem("Internals"))
        {
            if (ImGui::BeginChild("InternalsScroll")) {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Relations");
                DRAW_PTR("Driver", pVeh->m_pDriver);
                DRAW_PTR("Attached To", pVeh->m_pAttachedTo);
                DRAW_PTR("Tractor", pVeh->m_pTractor);
                DRAW_PTR("Trailer", pVeh->m_pTrailer);
                DRAW_PTR("Damage Entity", pVeh->m_pDamageEntity);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Timers & Bits");
                ImGui::Columns(2, nullptr, false);
                VAL_FIELD("Gun Firing", pVeh->m_nGunFiringTime);
                VAL_FIELD("Need Car Time", pVeh->m_nTimeTillWeNeedThisCar);
                ImGui::NextColumn();
                VAL_FIELD("Bomb Timer", pVeh->m_wBombTimer);
                VAL_FIELD("BlowUp Time", pVeh->m_nTimeWhenBlowedUp);
                ImGui::Columns(1);

                BF_MBIT("Bomb Type", pVeh->m_nBombOnBoard, 5);
                VAL_FIELD("Ammo Clip", pVeh->m_nAmmoInClip);
                VAL_FIELD("Weapon Use", pVeh->m_nVehicleWeaponInUse);

                ImGui::Separator();
                if (ImGui::TreeNode("Render Lights")) {
                    BF_BOOL("RF", pVeh->m_renderLights.m_bRightFront);
                    BF_BOOL("LF", pVeh->m_renderLights.m_bLeftFront);
                    BF_BOOL("RR", pVeh->m_renderLights.m_bRightRear);
                    BF_BOOL("LR", pVeh->m_renderLights.m_bLeftRear);
                    ImGui::TreePop();
                }
                BF_MBIT("Override Lights", pVeh->m_nOverrideLights, 2);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}


void Tab_Inspector() {
    if (ImGui::BeginTabItem("Inspector")) {
        ImGui::BeginChild("dasdasd");
        DrawMonsterInspector();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}