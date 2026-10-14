#include "pch.h"
#include "studio.h"
#include "studio_config.h"
#include "studio_file.h"
#include "studio_live.h"
#include "studio_policy.h"
#include "studio_theme.h"
#include "studio_workshop.h"
#include "studio_model.h"
#include <CStreaming.h>
#include "loader.h"
#include "defines.h"
#include "utils/datamgr.h"
#include "utils/samp.h"
#include <CPad.h>
#include <CCamera.h>
#include <CMenuManager.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <backends/imgui_impl_dx9.h>
#include <backends/imgui_impl_win32.h>
#include <d3d9.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {
using Json = StudioConfig::Json;
enum class Kind { Boolean, Integer, Float, Text };
struct IniSpec { const char *section, *key, *fallback, *help; Kind kind; };
constexpr IniSpec iniSpecs[] = {
#include "studio_ini.inc"
};
struct IniRow {
    const IniSpec *spec;
    std::string value, original;
    bool queued = false;
    bool restart = false;
    std::string label, listText;
};
std::vector<IniRow> iniRows;
struct ModelChoice { int id; char label[48]; };
std::vector<ModelChoice> models;
StudioConfig::IniChanges iniChanges;
std::string iniSource, status, workshopError;
bool iniExisted = false;
StudioConfig::IniChanges bootSettings;
const IniSpec *bindingTarget = nullptr;
int pendingBinding = 0;
struct ModelDraft {
    int model = 411;
    Json value = Json::object(), original = Json::object(), applied = Json::object();
    std::filesystem::path path;
    std::string source;
    bool existed = false, hadData = false, dirty = false, queued = false;
} draft;
bool modelLoaded = false, open = false, lost = false;
bool loadRequested = false, iniSaveRequested = false, modelSaveRequested = false;
bool iniRevertRequested = false, modelRevertRequested = false, iniReloadRequested = false, jsonReloadRequested = false;
bool filesReloaded = false, iniConflict = false, modelConflict = false;
bool addSectionRequested = false;
constexpr unsigned short controlLock = 1u << 1; // SDK's unused control bit; retain mission/cutscene bits.
HWND window = nullptr;
WNDPROC originalWndProc = nullptr;
ImGuiContext *context = nullptr;
bool ownsControlBit = false;
ImVec2 mousePosition;
CMouseControllerState mouseState{};
float mouseWheel = 0;
ImGuiTextFilter iniFilter, jsonFilter, modelFilter;
std::string templateName = "neon";
std::vector<std::string> workshopFrames, workshopAudio;
StudioSupport::Model workshopSupport;
RpClump *inspectedClump=nullptr;
unsigned inspectTime=0;
int requestedModel = 411;
int page = 0;
StudioTheme::Preferences appearance;
bool styleDirty = true, appearanceLoaded = false, appearanceSaveRequested = false, appearanceExisted = false;
std::string appearanceSource;
std::filesystem::path AppearancePath() {
    return std::filesystem::path(gConfig.GetIniPath()).parent_path() / "ModelExtras" / "studio.json";
}

std::string SettingLabel(const char *key) {
    std::string label;
    for (const char *p = key; *p; ++p) {
        if (*p == '_') { label += ' '; continue; }
        if (p != key && *p >= 'A' && *p <= 'Z'
            && ((p[-1] >= 'a' && p[-1] <= 'z') || (p[1] >= 'a' && p[1] <= 'z' && p[-1] >= 'A' && p[-1] <= 'Z'))) label += ' ';
        label += *p;
    }
    return label;
}

struct ContextScope {
    ImGuiContext *previous = ImGui::GetCurrentContext();
    ContextScope() { ImGui::SetCurrentContext(context); }
    ~ContextScope() { ImGui::SetCurrentContext(previous); }
};

void ClearMouse() {
    CPad::ClearMouseHistory();
    CPad::NewMouseControllerState = {};
    CPad::PCTempMouseControllerState = {};
}

void SetOpen(bool value) {
    if (open == value) return;
    open = value;
    bindingTarget = nullptr; pendingBinding = 0;
    if (context) {
        ContextScope scope;
        ImGui::GetIO().MouseDrawCursor = open;
        ImGui::GetIO().ClearInputKeys();
        ImGui::GetIO().ClearEventsQueue();
    }
    auto *pad = CPad::GetPad(0);
    if (open) {
        RECT client{};
        GetClientRect(window, &client);
        mousePosition = ImVec2(client.right * 0.5f, client.bottom * 0.5f);
        mouseState = {}; mouseWheel = 0;
        ownsControlBit = pad && !(pad->DisablePlayerControls & controlLock);
        if (pad) pad->DisablePlayerControls |= controlLock;
        if (iniRows.empty()) loadRequested = true;
    } else {
        if (pad && ownsControlBit) pad->DisablePlayerControls &= ~controlLock;
        ownsControlBit = false;
    }
    ClearMouse();
    CPad::NewKeyState = {}; CPad::OldKeyState = {};
}

bool ExclusiveMouse() {
    return RsGlobal.ps && RsGlobal.ps->fullScreen && RsGlobal.ps->diMouse;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wparam)) SetOpen(false);
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !(lparam & (1L << 30))) {
        if (wparam == 'Z' && (GetAsyncKeyState(VK_LCONTROL) & 0x8000) && !SAMP::IsInputActive() && !CTimer::m_UserPause && !CTimer::m_CodePause) {
            SetOpen(!open);
            return 0;
        }
        if (open && bindingTarget) {
            if (wparam == VK_ESCAPE) { bindingTarget = nullptr; pendingBinding = 0; }
            else if (wparam > VK_MBUTTON && wparam < 256) pendingBinding = static_cast<int>(wparam);
            return 0;
        }
        if (wparam == VK_ESCAPE && open) { SetOpen(false); return 0; }
    }
    if (context && open) {
        ContextScope scope;
        // Fullscreen GTA uses an exclusive DirectInput mouse, which may not send WM_MOUSE messages.
        if (!ExclusiveMouse() || message < WM_MOUSEFIRST || message > WM_MOUSELAST)
            ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);
    }
    if (open) {
        if (message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT) { SetCursor(nullptr); return TRUE; }
        if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
            || (message >= WM_KEYFIRST && message <= WM_KEYLAST) || message == WM_INPUT) return 0;
    }
    return CallWindowProcW(originalWndProc, hwnd, message, wparam, lparam);
}

void LoadIni() {
    iniSource = StudioFile::Read(gConfig.GetIniPath());
    iniExisted = std::filesystem::exists(gConfig.GetIniPath());
    iniChanges.clear();
    bindingTarget = nullptr; pendingBinding = 0;
    iniRows.clear(); iniRows.reserve(std::size(iniSpecs));
    CIniReader saved(gConfig.GetIniPath());
    for (const auto &spec : iniSpecs) {
        auto value = gConfig.data.get(spec.section, spec.key, spec.fallback);
        const bool restart = StudioPolicy::RestartReason(spec.section, spec.key) != nullptr
            || (std::string_view(spec.key) == "Carcols" && SAMP::IsPresent());
        if (restart) value = saved.data.get(spec.section, spec.key, spec.fallback);
        iniRows.push_back({&spec, value, value, false, restart, SettingLabel(spec.key)});
        if (spec.kind == Kind::Text) {
            try { iniRows.back().listText = StudioConfig::ModelList(value, true); }
            catch (const std::exception &) { iniRows.back().listText = value; }
        }
    }
    if (models.empty()) {
        models.reserve(512);
        for (int id = 1; id < 20000; ++id) {
            auto *info = CModelInfo::GetModelInfo(id);
            if (!info || info->GetModelType() != MODEL_INFO_VEHICLE) continue;
            ModelChoice choice{id};
            std::snprintf(choice.label, sizeof(choice.label), "%d | %.8s", id, static_cast<CVehicleModelInfo *>(info)->m_szGameName);
            models.push_back(choice);
        }
    }
}

void LoadModel(int model) {
    if (model <= 0 || model >= 20000) throw std::runtime_error("Model ID must be 1..19999");
    auto *info = CModelInfo::GetModelInfo(model);
    if (!info || info->GetModelType() != MODEL_INFO_VEHICLE) throw std::runtime_error("Select a vehicle model ID");
    workshopAudio.clear();
    std::error_code audioError;
    auto audioPath=std::filesystem::path(gConfig.GetIniPath()).parent_path()/"ModelExtras"/"audio";
    for (std::filesystem::directory_iterator it(audioPath,audioError),end; it!=end && !audioError && workshopAudio.size()<512; it.increment(audioError)) {
        if (it->is_regular_file(audioError)) workshopAudio.push_back(it->path().filename().string());
    }
    std::sort(workshopAudio.begin(),workshopAudio.end());
    workshopSupport=StudioModel::Inspect(static_cast<CVehicleModelInfo *>(info),model,workshopFrames);
    inspectedClump=info->m_pRwClump;
    // Request normal asynchronous streaming; never synchronously load a DFF in the UI.
    if (!inspectedClump) CStreaming::RequestModel(model,0);
    draft = {};
    draft.model = model;
    draft.hadData = DataMgr::Has(model);
    if (const auto *data = DataMgr::Find(model)) draft.value = draft.original = draft.applied = *data;
    auto source = DataMgr::GetPath(model);
    draft.path = source.empty()
        ? std::filesystem::path(gConfig.GetIniPath()).parent_path() / "ModelExtras" / "data" / (std::to_string(model) + ".jsonc")
        : std::filesystem::path(source);
    if (draft.path.extension() != ".jsonc")
        draft.path = std::filesystem::path(gConfig.GetIniPath()).parent_path() / "ModelExtras" / "data" / (std::to_string(model) + ".jsonc");
    draft.existed = std::filesystem::exists(draft.path);
    draft.source = StudioFile::Read(draft.path);
    workshopError.clear(); status.clear();
    requestedModel = model;
    modelLoaded = true;
}

void QueueModel() {
    draft.dirty = draft.value != draft.original;
    draft.queued = true;
}


void IniTab() {
    ImGui::TextUnformatted("Settings");
    ImGui::Spacing();
    if (StudioTheme::PrimaryButton("Save changes")) iniSaveRequested = true;
    StudioTheme::SameLineFor("Revert");
    if (ImGui::Button("Revert")) iniRevertRequested = true;
    StudioTheme::SameLineFor("Reload INI");
    if (ImGui::Button("Reload INI")) {
        if (!iniChanges.empty()) status = "Save or revert INI changes before reloading.";
        else iniReloadRequested = true;
    }
    StudioTheme::SameLineFor("0 pending"); ImGui::TextDisabled("%zu pending", iniChanges.size());
    if (!status.empty()) {
        ImGui::SameLine(0, 12 * appearance.scale);
        ImGui::TextUnformatted(status.data(), status.data() + std::min(status.find('\n'), status.size()));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", status.c_str());
    }
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##findsetting", "Search all settings...", iniFilter.InputBuf, sizeof(iniFilter.InputBuf))) iniFilter.Build();
    const char *sections[] = {"FEATURES", "LIGHTS", "SOUND", "KEYS", "TABLE", "CONFIG"};
    const char *labels[] = {"Features", "Lighting", "Audio", "Key bindings", "Model lists", "Startup"};
    for (int category = 0; category < 6; ++category) {
        // Fixed stack storage; all matches are drawn, never allocated on slider movement.
        IniRow *matches[std::size(iniSpecs)]{};
        int count = 0;
        for (auto &row : iniRows)
            if (std::string_view(row.spec->section) == sections[category] && iniFilter.PassFilter(row.spec->key)) matches[count++] = &row;
        if (!count) continue;
        const bool paired = category == 0 && ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 38;
        const int half = paired ? (count + 1) / 2 : count;
        ImGui::Spacing(); ImGui::TextColored(appearance.accent, "%s", labels[category]);
        ImGui::PushID(sections[category]);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10 * appearance.scale, 3 * appearance.scale));
        if (ImGui::BeginTable("settings", paired ? 4 : 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingStretchProp)) {
            for (int column = 0; column < (paired ? 2 : 1); ++column) {
                ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch, paired ? .86f : .55f);
                ImGui::TableSetupColumn("Value", paired ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch, paired ? ImGui::GetFrameHeight() * .72f * 1.85f : .45f);
            }
            for (int index = 0; index < half; ++index) {
                ImGui::TableNextRow();
                for (int column = 0; column < (paired ? 2 : 1); ++column) {
                    const int selected = index + column * half;
                    if (selected >= count) break;
                    auto &row = *matches[selected];
                    const auto &spec = *row.spec;
                    ImGui::PushID(spec.key);
                    ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
                    ImGui::TextWrapped("%s", row.label.c_str());
                    StudioTheme::Hint(spec.help);
                    if (row.restart) {
                        const char *note = std::string_view(spec.key) == "Carcols" ? "Unavailable in SA-MP" : "Restart required";
                        ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * .75f);
                        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(note).x
                            <= ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x) ImGui::SameLine();
                        ImGui::TextColored(ImVec4(.76f, .65f, .44f, 1), "%s", note);
                        ImGui::PopFont();
                        StudioTheme::Hint(StudioPolicy::RestartReason(spec.section, spec.key));
                    }
                    ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
                    bool changed = false;
                    if (spec.kind == Kind::Boolean) {
                        bool value = row.value != "0" && ::_stricmp(row.value.c_str(), "false") != 0;
                        ImGui::BeginDisabled(std::string_view(spec.key) == "Carcols" && SAMP::IsPresent());
                        changed = StudioTheme::Toggle("##value", &value);
                        ImGui::EndDisabled();
                        StudioTheme::Hint(spec.help);
                        if (changed) row.value = value ? "1" : "0";
                        if (!paired) { ImGui::SameLine(); ImGui::TextDisabled("%s", value ? "Enabled" : "Disabled"); }
                    } else if (spec.kind == Kind::Integer) {
                        int value;
                        try { value = std::stoi(row.value, nullptr, row.value.starts_with("0x") || row.value.starts_with("0X") ? 16 : 10); }
                        catch (...) { value = std::stoi(spec.fallback, nullptr, 0); }
                        if (std::string_view(spec.section) == "KEYS") {
                            char name[128]{};
                            wchar_t wideName[64]{};
                            const auto scan = MapVirtualKeyA(value, MAPVK_VK_TO_VSC);
                            const bool extended = value == VK_LEFT || value == VK_RIGHT || value == VK_UP || value == VK_DOWN
                                || value == VK_INSERT || value == VK_DELETE || value == VK_HOME || value == VK_END
                                || value == VK_PRIOR || value == VK_NEXT || value == VK_DIVIDE || value == VK_NUMLOCK;
                            if (GetKeyNameTextW(static_cast<LONG>((scan << 16) | (extended ? 1u << 24 : 0)), wideName, std::size(wideName)))
                                WideCharToMultiByte(CP_UTF8, 0, wideName, -1, name, sizeof(name), nullptr, nullptr);
                            else std::snprintf(name, sizeof(name), "Key 0x%02X", value);
                            if (StudioTheme::Keycap("##key", bindingTarget == &spec ? "..." : name, bindingTarget == &spec)) {
                                bindingTarget = &spec; pendingBinding = 0;
                            }
                            if (bindingTarget == &spec) { ImGui::SameLine(); ImGui::TextDisabled("Press a key / ESC cancels"); }
                        } else changed = StudioTheme::Slider("##value", ImGuiDataType_S32, &value, 0, 255, "%d");
                        if (changed) row.value = std::to_string(value);
                    } else if (spec.kind == Kind::Float) {
                        float value, max = 10.0f;
                        try { value = std::stof(row.value); } catch (...) { value = std::stof(spec.fallback); }
                        std::string_view key(spec.key);
                        if (key == "SoundMult" || key == "PointLightIntensity") max = 1.0f;
                        else if (key == "LightShadowDistance") max = 500.0f;
                        else if (key == "HighBeamPointLightMul" || key == "SirenPointLightMul") max = 4.0f;
                        max = std::max(max, value);
                        changed = StudioTheme::Slider("##value", ImGuiDataType_Float, &value, key == "HighBeamPointLightMul" ? 1.0f : key == "LightHeightLimit" ? std::min(-10.0f,value) : 0.0f, max, "%.3f");
                        if (changed) row.value = std::to_string(value);
                    } else {
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
                        if (ImGui::InputTextMultiline("##value", &row.listText, ImVec2(-1, ImGui::GetTextLineHeight() * 3 + ImGui::GetStyle().FramePadding.y * 2))) {
                            row.value = row.listText;
                            auto key = std::make_pair(std::string(spec.section), std::string(spec.key));
                            if (row.value == row.original) iniChanges.erase(key); else iniChanges[key] = row.value;
                        }
                        changed = ImGui::IsItemDeactivatedAfterEdit();
                        ImGui::PopStyleVar();
                    }
                    StudioTheme::Hint(spec.help);
                    if (changed) {
                        row.queued = !row.restart;
                        auto key = std::make_pair(std::string(spec.section), std::string(spec.key));
                        if (row.value == row.original) iniChanges.erase(key); else iniChanges[key] = row.value;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar(); ImGui::PopID();
    }
}

void ModelInformation(const char *vehicleName) {
    const Json empty = Json::object();
    auto it = draft.value.find("metadata");
    const Json &metadata = it != draft.value.end() && it->is_object() ? *it : empty;
    const auto field = [&](std::string_view name) -> const Json * {
        auto value = metadata.find(std::string(name));
        return value == metadata.end() ? nullptr : &*value;
    };
    const auto display = [&](std::string_view name) {
        const auto *value = field(name);
        if (!value || value->is_null() || (value->is_string() && value->get_ref<const std::string &>().empty())) return std::string("Not specified");
        if (name == "minver" && value->is_number_integer()) {
            const int number = value->get<int>();
            const auto match = std::find_if(StudioSchema::MinimumVersions.begin(),StudioSchema::MinimumVersions.end(),[&](const auto &entry) { return entry.first == number; });
            return match == StudioSchema::MinimumVersions.end() ? "Unknown (" + std::to_string(number) + ")" : std::string(match->second);
        }
        return value->is_string() ? value->get<std::string>() : value->dump();
    };

    const std::string description=display("desc");
    const float lineHeight=ImGui::GetTextLineHeightWithSpacing();
    const float descriptionWidth=std::max(80.0f,ImGui::GetContentRegionAvail().x*.34f);
    const float descriptionHeight=ImGui::CalcTextSize(description.c_str(),nullptr,false,descriptionWidth).y;
    size_t extraFields=0;
    for (auto item=metadata.begin();item!=metadata.end();++item) {
        const auto &key=item.key();
        if (key!="author" && key!="version" && key!="creationtime" && key!="desc" && key!="minver") ++extraFields;
    }
    const float cardHeight=lineHeight*6.2f+std::max(0.0f,descriptionHeight-lineHeight)+lineHeight*static_cast<float>(extraFields);
    ImGui::BeginChild("##modelInformation",ImVec2(0,cardHeight),ImGuiChildFlags_Borders,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::TextColored(appearance.accent,"MODEL INFORMATION");
    ImGui::SameLine(); ImGui::TextUnformatted(vehicleName);
    if (ImGui::BeginTable("metadataSummary",4,ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthStretch,.14f);
        ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,.36f);
        ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthStretch,.19f);
        ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,.31f);
        const auto pair = [&](const char *leftLabel, const std::string &left, const char *rightLabel, const std::string &right) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s",leftLabel);
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s",left.c_str());
            ImGui::TableNextColumn(); ImGui::TextDisabled("%s",rightLabel);
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s",right.c_str());
        };
        pair("Author",display("author"),"Vehicle version",display("version"));
        pair("Created",display("creationtime"),"Minimum ModelExtras",display("minver"));
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::TextDisabled("Description");
        ImGui::TableNextColumn(); ImGui::TextWrapped("%s",description.c_str());
        ImGui::EndTable();
    }
    for (auto item=metadata.begin();item!=metadata.end();++item) {
        const auto &key=item.key();
        if (key=="author" || key=="version" || key=="creationtime" || key=="desc" || key=="minver") continue;
        const auto value=item->is_string() ? item->get<std::string>() : item->dump();
        ImGui::TextDisabled("%s:",StudioWorkshop::Label(key).c_str()); ImGui::SameLine(); ImGui::TextWrapped("%s",value.c_str());
    }
    ImGui::EndChild();
}

void ModelTab() {
    ImGui::TextUnformatted("Vehicle workshop"); ImGui::Spacing();
    ImGui::BeginDisabled(draft.dirty);
    const char *selected = "Select a vehicle";
    for (const auto &model : models) if (model.id == requestedModel) { selected = model.label; break; }
    if (ImGui::BeginCombo("Vehicle", selected)) {
        modelFilter.Draw("Find vehicle / ID");
        for (const auto &model : models) if (modelFilter.PassFilter(model.label))
            if (ImGui::Selectable(model.label, model.id == requestedModel)) { requestedModel = model.id; loadRequested = true; }
        ImGui::EndCombo();
    }
    StudioTheme::SameLineFor("Use current vehicle");
    if (ImGui::Button("Use current vehicle")) {
        if (auto *vehicle = FindPlayerVehicle(-1, false)) { requestedModel = vehicle->m_nModelIndex; loadRequested = true; }
        else { status = "Enter a vehicle or select a model ID."; workshopError = status; }
    }
    ImGui::EndDisabled();
    if (!modelLoaded) return;
    ImGui::Text("Editing model %d | %s", draft.model, draft.dirty ? "Unsaved" : "No pending changes");
    if (draft.dirty) ImGui::TextDisabled("Save or revert before selecting another model.");
    StudioTheme::SameLineFor("Save JSONC");
    if (StudioTheme::PrimaryButton("Save JSONC")) modelSaveRequested = true;
    StudioTheme::SameLineFor("Revert");
    if (ImGui::Button("Revert")) modelRevertRequested = true;
    StudioTheme::SameLineFor("Reload JSONC");
    if (ImGui::Button("Reload JSONC")) {
        if (draft.dirty) { status = "Save or revert JSONC changes before reloading."; workshopError = status; }
        else jsonReloadRequested = true;
    }
    ModelInformation(selected);
    ImGui::Separator();
    if (!workshopSupport.inspected) ImGui::TextDisabled(workshopSupport.limited ? "Model exceeds the inspection limit. DFF-dependent settings are hidden." : "Waiting for model streaming. DFF-dependent settings appear after inspection.");
    if (!workshopSupport.Section(templateName)) templateName="neon";
    const auto featureRow=ImGui::GetCursorScreenPos();
    const float featureRight=featureRow.x+ImGui::GetContentRegionAvail().x;
    if (ImGui::BeginCombo("Feature",StudioWorkshop::Label(templateName).c_str())) {
        for (const auto &[name,value] : StudioSchema::Sections().items()) {
            bool available = workshopSupport.Section(name);
            if (available && ImGui::Selectable(StudioWorkshop::Label(name).c_str(),name==templateName)) templateName=name;
        }
        ImGui::EndCombo();
    }
    if (draft.value.contains(templateName) && StudioWorkshop::RemoveFeatureButton(featureRow,featureRight)) {
        draft.value.erase(templateName);
        QueueModel();
    }
    ImGui::TextWrapped("%s",StudioSchema::Help(templateName));
    if (draft.value.contains(templateName)) {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##findfield","Search this feature...",jsonFilter.InputBuf,sizeof(jsonFilter.InputBuf))) jsonFilter.Build();
        std::vector<std::string> path{templateName}; unsigned shown=0;
        if (StudioWorkshop::Edit(draft.value[templateName],path,draft.value,jsonFilter,workshopFrames,workshopAudio,shown,workshopSupport)) QueueModel();
        if (shown>=2000) ImGui::TextDisabled("Collapse groups to browse this large config.");
    } else if (ImGui::Button("Add settings")) addSectionRequested = true;
    ImGui::TextDisabled("Use X beside a setting to remove it. CTRL + click a slider for an exact value.");
}

void AppearanceTab() {
    ImGui::TextUnformatted("Appearance"); ImGui::Spacing();
    ImGui::TextColored(appearance.accent, "ACCENT COLOR");
    const ImVec4 presets[] = {{.43f, .52f, .72f, 1}, {.43f, .47f, .69f, 1}, {.58f, .45f, .65f, 1}, {.67f, .49f, .32f, 1}, {.64f, .39f, .46f, 1}};
    for (int i = 0; i < 5; ++i) {
        ImGui::PushID(i);
        if (ImGui::ColorButton("##accent", presets[i], ImGuiColorEditFlags_NoTooltip, ImVec2(44 * appearance.scale, 30 * appearance.scale))) {
            appearance.accent = presets[i]; styleDirty = true;
        }
        if (i != 4) ImGui::SameLine();
        ImGui::PopID();
    }
    styleDirty |= ImGui::ColorEdit3("Custom accent", &appearance.accent.x, ImGuiColorEditFlags_NoInputs);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(appearance.accent, "READABILITY & SPACING");
    styleDirty |= ImGui::SliderFloat("Interface scale", &appearance.scale, .8f, 1.4f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    styleDirty |= ImGui::SliderFloat("Font size", &appearance.fontSize, 14, 22, "%.0f px", ImGuiSliderFlags_AlwaysClamp);
    float opacity = appearance.opacity * 100;
    if (ImGui::SliderFloat("Opacity", &opacity, 40, 100, "%.0f%%", ImGuiSliderFlags_AlwaysClamp)) {
        appearance.opacity = opacity / 100; styleDirty = true;
    }
    ImGui::Spacing();
    if (StudioTheme::PrimaryButton("Save appearance")) appearanceSaveRequested = true;
    StudioTheme::SameLineFor("Reset appearance");
    if (ImGui::Button("Reset appearance")) { appearance = {}; styleDirty = true; }
    ImGui::TextWrapped("Appearance previews immediately. Save keeps these preferences for your next session; it does not change vehicle or feature settings.");
}

void AboutTab() {
    StudioTheme::Heading("About ModelExtras", "More life, detail and character for San Andreas vehicles.");
    ImGui::TextColored(appearance.accent, "MODELEXTRAS %s", MOD_VERSION);
    ImGui::TextUnformatted("By Grinch_, Caner Karaca, Ameer and KaiQ");
    ImGui::TextWrapped("Studio brings feature tuning and a vehicle configuration workshop into the game. Changes can be previewed live and saved when you are ready.");
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(appearance.accent, "QUICK GUIDE");
    ImGui::BulletText("Left CTRL + Z opens or closes Studio. ESC closes it.");
    ImGui::BulletText("CTRL + click a slider to enter an exact value.");
    ImGui::BulletText("Save keeps changes. Revert restores loaded settings.");
    ImGui::BulletText("Vehicle options use the model's existing JSONC capabilities.");
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextColored(appearance.accent, "PROJECT & ACKNOWLEDGEMENTS");
    ImGui::TextWrapped("%s", GITHUB_LINK);
    if (ImGui::Button("Copy project link")) ImGui::SetClipboardText(GITHUB_LINK);
    ImGui::Text("Dear ImGui %s by Omar Cornut and contributors", IMGUI_VERSION);
    ImGui::TextWrapped("ModelExtras and Dear ImGui are MIT licensed. Built with Plugin-SDK, RenderWare, nlohmann/json, CIniReader and AixLog. Segoe UI is loaded from Windows when available.");
}

void Draw() {
    if (!context) {
        window = RsGlobal.ps ? RsGlobal.ps->window : nullptr;
        auto *device = static_cast<IDirect3DDevice9 *>(RwD3D9GetCurrentD3DDevice());
        if (!window || !device) return;
        auto *previous = ImGui::GetCurrentContext();
        IMGUI_CHECKVERSION();
        context = ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().LogFilename = nullptr;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        StudioTheme::LoadFont();
        StudioTheme::Apply(appearance);
        styleDirty = false;
        bool win32 = ImGui_ImplWin32_Init(window);
        bool dx9 = win32 && ImGui_ImplDX9_Init(device);
        if (dx9) originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
        if (!originalWndProc) {
            if (dx9) ImGui_ImplDX9_Shutdown();
            if (win32) ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(context); context = nullptr;
        }
        ImGui::SetCurrentContext(previous);
    }
    if (open && (!Util::IsWindowFocused() || SAMP::IsInputActive() || CTimer::m_UserPause || CTimer::m_CodePause)) SetOpen(false);
    if (!context || !open || lost) return;
    auto *device = static_cast<IDirect3DDevice9 *>(RwD3D9GetCurrentD3DDevice());
    if (!device || device->TestCooperativeLevel() != D3D_OK) return;
    ContextScope scope;
    if (styleDirty) { StudioTheme::Apply(appearance); styleDirty = false; }
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    if (ExclusiveMouse()) {
        auto &io = ImGui::GetIO();
        io.AddMousePosEvent(mousePosition.x, mousePosition.y);
        const bool buttons[] = {mouseState.lmb != 0, mouseState.rmb != 0, mouseState.mmb != 0, mouseState.bmx1 != 0, mouseState.bmx2 != 0};
        for (int i = 0; i < 5; ++i) io.AddMouseButtonEvent(i, buttons[i]);
        if (mouseWheel) io.AddMouseWheelEvent(0, std::exchange(mouseWheel, 0.0f));
    }
    ImGui::NewFrame();
    bool visible = open;
    StudioTheme::BeginShell(page, visible, MOD_VERSION);
    if (bindingTarget && (page != 0 || ImGui::IsMouseClicked(0))) { bindingTarget = nullptr; pendingBinding = 0; }
    if (page == 1 && !workshopError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(.25f, .075f, .085f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(.72f, .20f, .23f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f * appearance.scale);
        if (ImGui::BeginChild("##workshopError", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders)) {
            ImGui::TextColored(ImVec4(1.0f, .55f, .55f, 1.0f), "This change was not applied");
            ImGui::TextWrapped("%s", workshopError.c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(); ImGui::PopStyleColor(2);
    } else if (page != 0 && !status.empty()) { ImGui::TextWrapped("%s", status.c_str()); ImGui::Separator(); }
    if (iniRows.empty()) ImGui::TextUnformatted("Loading settings...");
    else {
        if (page == 0) IniTab();
        if (page == 1) {
            try { ModelTab(); } catch (const std::exception &error) { status = workshopError = error.what(); }
        }
        if (page == 2) AppearanceTab();
        if (page == 3) AboutTab();
    }
    StudioTheme::EndShell(status.c_str());
    if (!visible) SetOpen(false);
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}
}

void Studio::PreserveRestartSettings() {
    StudioPolicy::Restore(gConfig.data, bootSettings);
}
bool Studio::IsOpen() { return open; }
void Studio::FilesReloaded() { filesReloaded = true; }

void Studio::Tick() {
    if (open && (!Util::IsWindowFocused() || SAMP::IsInputActive() || CTimer::m_UserPause || CTimer::m_CodePause)) SetOpen(false);
    if (open) {
        if (ExclusiveMouse()) {
            mouseState = CPad::NewMouseControllerState;
            RECT client{};
            GetClientRect(window, &client);
            mousePosition.x = std::clamp(mousePosition.x + mouseState.x * (CMenuManager::bInvertMouseX ? -1.0f : 1.0f), 0.0f, static_cast<float>(std::max(0L, client.right - 1)));
            mousePosition.y = std::clamp(mousePosition.y + mouseState.y * (CMenuManager::bInvertMouseY ? -1.0f : 1.0f), 0.0f, static_cast<float>(std::max(0L, client.bottom - 1)));
            mouseWheel += static_cast<float>(mouseState.wheelUp != 0) - static_cast<float>(mouseState.wheelDown != 0);
        }
        if (auto *pad = CPad::GetPad(0)) {
            pad->DisablePlayerControls |= controlLock;
            pad->NewState = {}; pad->OldState = {};
        }
        ClearMouse();
        CPad::NewKeyState = {}; CPad::OldKeyState = {};
    }
    try {
        if (open && !appearanceLoaded) {
            appearanceLoaded = true;
            appearanceExisted = std::filesystem::exists(AppearancePath());
            appearanceSource = StudioFile::Read(AppearancePath());
            if (!appearanceSource.empty()) {
                const auto data = StudioConfig::Parse(appearanceSource);
                if (!data.is_object()) throw std::runtime_error("Invalid Studio appearance file; defaults retained.");
                auto next = StudioTheme::Preferences{};
                next.scale = data.value("scale", next.scale);
                next.fontSize = data.value("font_size", next.fontSize);
                next.opacity = data.value("opacity", next.opacity);
                if (data.contains("accent")) {
                    const auto &color = data.at("accent");
                    if (!color.is_array() || color.size() != 3) throw std::runtime_error("Studio accent must contain three RGB values.");
                    next.accent = ImVec4(color.at(0).get<float>(), color.at(1).get<float>(), color.at(2).get<float>(), 1);
                }
                if (!std::isfinite(next.scale) || next.scale < .8f || next.scale > 1.4f
                    || !std::isfinite(next.fontSize) || next.fontSize < 14 || next.fontSize > 22
                    || !std::isfinite(next.opacity) || next.opacity < .4f || next.opacity > 1)
                    throw std::runtime_error("Studio scale, font size or opacity is out of range; defaults retained.");
                for (float channel : {next.accent.x, next.accent.y, next.accent.z})
                    if (!std::isfinite(channel) || channel < 0 || channel > 1) throw std::runtime_error("Studio accent is out of range; defaults retained.");
                appearance = next; styleDirty = true;
            }
        }
        if (appearanceSaveRequested) {
            appearanceSaveRequested = false;
            const Json data = {{"scale", appearance.scale}, {"font_size", appearance.fontSize}, {"opacity", appearance.opacity},
                {"accent", {appearance.accent.x, appearance.accent.y, appearance.accent.z}}};
            const auto output = data.dump(2) + "\n";
            StudioFile::Save(AppearancePath(), appearanceSource, appearanceExisted, output);
            appearanceSource = output; appearanceExisted = true;
            status = "Appearance saved.";
        }
        if (filesReloaded) {
            filesReloaded = false;
            if (iniChanges.empty()) LoadIni();
            else {
                iniConflict = true;
                for (auto &row : iniRows) row.queued = false;
            }
            if (modelLoaded) {
                if (!draft.dirty) LoadModel(draft.model);
                else {
                    modelConflict = true;
                    draft.queued = false;
                    draft.original = draft.applied = DataMgr::Has(draft.model) ? DataMgr::Get(draft.model) : Json::object();
                    draft.hadData = DataMgr::Has(draft.model);
                }
            }
            if (iniConflict || modelConflict) {
                status = "Files reloaded while Studio had edits. Drafts retained; revert the affected draft to resolve the conflict.";
                if (modelConflict) workshopError = status;
            }
        }
        if (loadRequested) {
            loadRequested = false;
            if (iniRows.empty()) LoadIni();
            if (!modelLoaded || !draft.dirty) LoadModel(requestedModel);
        }
        if (modelLoaded && open && page==1 && CTimer::m_snTimeInMilliseconds-inspectTime>=500) {
            inspectTime=CTimer::m_snTimeInMilliseconds;
            auto *info=CModelInfo::GetModelInfo(draft.model);
            if (info && info->GetModelType()==MODEL_INFO_VEHICLE) {
                if (info->m_pRwClump!=inspectedClump) {
                    workshopSupport=StudioModel::Inspect(static_cast<CVehicleModelInfo *>(info),draft.model,workshopFrames);
                    inspectedClump=info->m_pRwClump;
                }
            }
        }
        if (iniRevertRequested) {
            iniRevertRequested = false;
            if (iniConflict) { LoadIni(); iniConflict = false; }
            else for (auto &row : iniRows) {
                row.queued = !row.restart && (row.queued || row.value != row.original);
                row.value = row.original;
                if (row.spec->kind == Kind::Text) {
                    try { row.listText = StudioConfig::ModelList(row.value, true); }
                    catch (const std::exception &) { row.listText = row.value; }
                }
            }
            iniChanges.clear();
        }
        if (bindingTarget && pendingBinding) {
            for (auto &row : iniRows) if (row.spec == bindingTarget) {
                row.value = std::to_string(pendingBinding); row.queued = !row.restart;
                auto key = std::make_pair(std::string(row.spec->section), std::string(row.spec->key));
                if (row.value == row.original) iniChanges.erase(key); else iniChanges[key] = row.value;
            }
            bindingTarget = nullptr; pendingBinding = 0;
        }
        for (auto &row : iniRows) if (row.queued) {
            row.queued = false;
            if (iniConflict) throw std::runtime_error("Revert the INI draft after reloading files before previewing new edits.");
            if (row.spec->kind == Kind::Text) {
                row.value = StudioConfig::ModelList(row.listText);
                auto key = std::make_pair(std::string(row.spec->section), std::string(row.spec->key));
                if (row.value == row.original) iniChanges.erase(key); else iniChanges[key] = row.value;
            }
            StudioConfig::ValidateIniValue(row.value, row.spec->kind == Kind::Float, row.spec->kind == Kind::Text);
            gConfig.data.set(row.spec->section, row.spec->key, row.value);
            StudioLive::ApplyIni(row.spec->section, row.spec->key);
        }
        if (modelRevertRequested) {
            modelRevertRequested = false;
            if (modelConflict) { LoadModel(draft.model); modelConflict = false; }
            draft.value = draft.original;
            draft.dirty = false; draft.queued = true;
        }
        if (addSectionRequested) {
            addSectionRequested = false;
            if (templateName == "carcols") draft.value[templateName] = StudioConfig::Templates().at("carcols");
            else if (templateName == "sirens") draft.value[templateName] = Json{{"states",Json::object()}};
            else draft.value[templateName] = Json::object();
            QueueModel();
        }
        if (draft.queued && modelLoaded) {
            draft.queued = false;
            if (modelConflict) throw std::runtime_error("Revert the JSONC draft after reloading files before previewing new edits.");
            const auto &next = draft.value;
            StudioConfig::Validate(next);
            workshopError.clear(); status.clear();
            if (next != draft.applied) {
                DataMgr::SetPreview(draft.model, next);
                StudioLive::ApplyModel(draft.model, draft.applied, next);
                draft.applied = next;
            }
            if (!draft.hadData && next.empty()) DataMgr::RemovePreview(draft.model);
        }
        if (iniSaveRequested) {
            iniSaveRequested = false;
            if (iniConflict) throw std::runtime_error("INI draft conflicts with reloaded files; revert it first.");
            for (auto &row : iniRows) if (row.value != row.original) {
                if (row.spec->kind == Kind::Text) {
                    row.value = StudioConfig::ModelList(row.listText);
                    auto key = std::make_pair(std::string(row.spec->section), std::string(row.spec->key));
                    if (row.value == row.original) iniChanges.erase(key); else iniChanges[key] = row.value;
                }
                StudioConfig::ValidateIniValue(row.value, row.spec->kind == Kind::Float, row.spec->kind == Kind::Text);
            }
            auto output = StudioConfig::PatchIni(iniSource, iniChanges);
            StudioFile::Save(gConfig.GetIniPath(), iniSource, iniExisted, output);
            iniSource = output; iniExisted = true;
            for (auto &row : iniRows) row.original = row.value;
            iniChanges.clear(); status = "INI saved.";
        }
        if (modelSaveRequested && modelLoaded) {
            modelSaveRequested = false;
            if (modelConflict) throw std::runtime_error("JSONC draft conflicts with reloaded files; revert it first.");
            StudioConfig::Validate(draft.value);
            auto output = draft.value.dump(2) + "\n";
            StudioFile::Save(draft.path, draft.source, draft.existed, output);
            draft.source = output; draft.existed = true; draft.hadData = true;
            draft.original = draft.value; draft.dirty = false; draft.queued = true;
            workshopError.clear();
            status = "JSONC saved.";
        }
        if (iniReloadRequested) {
            iniReloadRequested = false;
            const auto previous = iniRows;
            gConfig.data.clear(); gConfig.SetIniPath();
            PreserveRestartSettings();
            LoadIni(); iniConflict = false;
            for (size_t index = 0; index < iniRows.size(); ++index)
                if (!iniRows[index].restart && previous[index].value != iniRows[index].value)
                    StudioLive::ApplyIni(iniRows[index].spec->section, iniRows[index].spec->key);
            status = "INI reloaded.";
        }
        if (jsonReloadRequested && modelLoaded) {
            jsonReloadRequested = false;
            if (!std::filesystem::exists(draft.path) && !draft.existed) throw std::runtime_error("Save this model as JSONC before reloading its file.");
            auto next = std::filesystem::exists(draft.path) ? StudioConfig::Parse(StudioFile::Read(draft.path)) : Json::object();
            StudioConfig::Validate(next);
            DataMgr::SetPreview(draft.model, next);
            StudioLive::ApplyModel(draft.model, draft.applied, next);
            if (!std::filesystem::exists(draft.path)) DataMgr::RemovePreview(draft.model);
            LoadModel(draft.model); modelConflict = false;
            workshopError.clear();
            status = "Selected model JSONC reloaded.";
        }

    } catch (const std::exception &error) {
        status = workshopError = error.what();
        draft.queued = false;
    }
}

void Studio::Init() {
    for (const auto &spec : iniSpecs) if (StudioPolicy::RestartReason(spec.section, spec.key)) {
        bootSettings[{spec.section, spec.key}] = gConfig.data.get(spec.section, spec.key, "");
    }
    Events::drawingEvent += [] {
        try { Draw(); } catch (const std::exception &error) {
            status = workshopError = error.what();
            // End an interrupted UI frame, then recover on the next drawing event.
            if (context) { ContextScope scope; ImGui::EndFrame(); }
        }
    };
    Events::d3dLostEvent.before += [] {
        lost = true;
        if (context) { ContextScope scope; ImGui_ImplDX9_InvalidateDeviceObjects(); }
    };
    Events::d3dResetEvent.after += [] { lost = false; };
    Events::shutdownRwEvent.before += [] {
        SetOpen(false);
        if (!context) return;
        ContextScope scope;
        if (window && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC)) == WndProc)
            SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc));
        ImGui_ImplDX9_Shutdown(); ImGui_ImplWin32_Shutdown();
        // The saved previous context may be ours when another mod has no context.
        if (scope.previous == context) scope.previous = nullptr;
        ImGui::DestroyContext(context); context = nullptr;
    };
}
