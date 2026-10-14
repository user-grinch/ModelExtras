#pragma once
#include "studio_support.h"
#include "studio_theme.h"
#include <imgui.h>
#include <cctype>
#include <set>
#include <misc/cpp/imgui_stdlib.h>

namespace StudioWorkshop {
using StudioConfig::Json;
inline std::string Label(std::string_view key) {
    static const Json labels = {{"metadata","Model information"},{"carcols","Paint colors"},{"exhausts","Exhaust effects"},{"gauges","Dashboard gauges"},{"leds","Dashboard LEDs"},{"lights","Vehicle lighting"},{"sound","Vehicle sounds"},{"plate","License plate"},{"rollback_bed","Rollback bed"},{"color","Color"},{"color_off","Off color"},{"minver","Minimum ModelExtras version"},
        {"author","Author"},{"version","Vehicle version"},{"desc","Description"},{"creationtime","Creation date"},{"strobedelay","Strobe interval (ms)"},{"rotationchecks","Check shadow direction"},
        {"maxrpm","Maximum RPM"},{"maxspeed","Maximum speed"},{"maxturbo","Maximum turbo"},{"maxrotation","Maximum rotation (degrees)"},
        {"minangle","Minimum angle (degrees)"},{"maxangle","Maximum angle (degrees)"},{"triggerspeed","Deployment speed"},
        {"kph","Use kilometres"},{"12hformat","12 hour clock"},{"movmul","Movement multiplier"},{"mul","Rotation multiplier"},
        {"popout","Pop-out distance"},{"nitro_effect","Nitro effect"},{"pattern","Flash timing (ms)"},{"colors","Color palette / sequence"},
        {"mode","Color mode"},{"requires_lights","Require vehicle lights"},{"smooth","Smooth transition"},
        {"reference","Reference preset"},{"diffuse","Material appearance"},{"state","Initially illuminated"},{"imvehft","ImVehFt material IDs"},
        {"angleoffset","Shadow angle offset"},{"rot_speed","Rotation speed"},{"target_rot","Target rotation"},{"move_speed","Movement speed"},{"target_move","Target movement"}};
    if (labels.contains(key)) return labels.at(key).get<std::string>();
    std::string s(key); for (auto &c : s) if (c == '_') c = ' ';
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] -= 32;
    return s;
}
inline bool Combo(Json &node, const char *label, const std::vector<std::string_view> &choices) {
    bool changed = false;
    auto value = node.is_string() ? node.get<std::string>() : std::string();
    if (ImGui::BeginCombo(label,value.empty() ? "Default" : value.c_str())) {
        std::vector<std::string> offered;
        for (auto choice : choices) {
            if (StudioSupport::HasNameInsensitive(offered,choice)) continue;
            offered.emplace_back(choice);
            const auto text=choice.empty() ? "Default" : choice.data();
            ImGui::PushID(text);
            if (ImGui::Selectable(text,value == choice)) { node = choice; changed = true; }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}
inline bool RemoveFeatureButton(const ImVec2 &rowStart, float right) {
    const auto next=ImGui::GetCursorScreenPos();
    const float unit=ImGui::GetStyle().FontScaleMain;
    const float margin=ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorScreenPos(ImVec2(std::max(rowStart.x,right-30*unit-margin),rowStart.y));
    const bool pressed=StudioTheme::CloseButton(unit);
    StudioTheme::Hint("Remove all settings in this feature.");
    ImGui::SetCursorScreenPos(next);
    return pressed;
}
inline bool Color(Json &node, const char *label, const Json &refs, bool referenceAllowed) {
    float c[4]{1,1,1,1};
    static const Json empty=Json::object();
    const bool decoded = StudioSchema::DecodeColor(node,c,referenceAllowed ? refs : empty);
    bool changed = false;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * .64f);
    if (ImGui::ColorEdit4(label,c,ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_DisplayHex)) {
        // A palette reference is replaced locally; never edit every user of it.
        StudioSchema::EncodeColor(node,c); changed = true;
    }
    if (!decoded) { ImGui::TextDisabled("Unresolved color; choose a color to override it."); }
    if (referenceAllowed && !refs.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Palette")) ImGui::OpenPopup("palette");
        if (ImGui::BeginPopup("palette")) {
            std::set<std::string,StudioSupport::NameLessInsensitive> offered;
            for (const auto &[name,v] : refs.items()) if (offered.insert(name).second) {
                ImGui::PushID(name.c_str());
                if (ImGui::Selectable(name.c_str(),node.is_string() && node == name)) { node = name; changed = true; }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
    }
    return changed;
}
inline Json ArrayItem(const std::vector<std::string> &p, const Json &node) {
    if (p.back()=="pattern") return 100;
    if (p.back()=="offset") return 0.0;
    if (p[0]=="sirens" && p.back()=="colors") return Json::array({1000,StudioSchema::Color()});
    if (p[0]=="carcols" && p.back()=="colors") return Json{{"red",255},{"green",255},{"blue",255}};
    if (p.back()=="variations") return StudioConfig::Templates().at("carcols").at("variations")[0];
    if (p.back()=="reference") return "";
    if (p.back()=="states") return Json::object();
    if (!node.empty()) return node.back();
    return Json();
}
inline bool Edit(Json &node, std::vector<std::string> &path, const Json &root, ImGuiTextFilter &filter,
                 const std::vector<std::string> &frames, const std::vector<std::string> &audio, unsigned &shown, const StudioSupport::Model &support, bool *remove = nullptr) {
    if (++shown > 2000 || path.size() > 32 || !support.Allows(path,&root)) return false;
    const auto &key = path.back();
    auto label = Label(key);
    if (path[0]=="sirens" && path.size()>1 && path[path.size()-2]=="pattern") label=node.is_array() ? "Repeat block "+key : "Interval "+key+" (ms)";
    if (path[0]=="sirens" && path.size()>2 && path[path.size()-3]=="pattern") label=key=="0" ? "Repeat count" : "Interval "+key+" (ms)";
    if (path[0]=="sirens" && key=="radius") label=path.size()>1 && path[path.size()-2]=="rotator" ? "Sweep (degrees)" : "Viewing angle (degrees)";
    if (path[0]=="sirens" && path.size()>2 && path[path.size()-3]=="colors") label=key=="0" ? "Hold (ms)" : "Color";
    if (path[0]=="neon" && key=="offset") label="Position offset";
    if (path[0]=="carcols" && path.size()==3 && path[1]=="colors") label="Color "+key;
    if (path[0]=="carcols" && path.size()==3 && path[1]=="variations") label="Paint variation "+key;
    ImGui::PushID(key.c_str());
    bool changed = false;
    static const Json empty = Json::object();
    const auto &refs = path[0]=="sirens" && root.contains("sirens") && root["sirens"].contains("references") && root["sirens"]["references"].contains("colors")
        ? root["sirens"]["references"]["colors"] : root.contains("colors") ? root["colors"] : empty;
    if (path.size()==1 && (path[0]=="neon" || path[0]=="spotlights" || path[0]=="spotlight") && StudioSchema::IsColor(node,path)) {
        if (ImGui::Button("Configure additional settings")) { node=Json{{"color",node}}; changed=true; }
        StudioTheme::Hint("Keep this color and use an object to add the other supported settings.");
    }
    const bool color=StudioSchema::IsColor(node,path);
    const bool leaf=color || node.is_primitive();
    bool row=false;
    if (leaf) {
        row=ImGui::BeginTable("field",remove ? 3 : 2,ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX);
        if (row) {
            ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthStretch,.38f);
            ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,.62f);
            if (remove) ImGui::TableSetupColumn("Remove",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight());
            ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
            ImGui::TextWrapped("%s",label.c_str()); ImGui::TableNextColumn(); ImGui::SetNextItemWidth(-1);
        }
    }
    if (color) {
        const bool plate = path[0]=="plate" || (path[0]=="lights" && path.size()>1 && path[1]=="plate");
        const bool sequence = path[0]=="sirens" && path.size()>2 && path[path.size()-3]=="colors" && key=="1";
        const bool refAllowed = !plate && path[0] != "neon" && path[0] != "plate" && path[0] != "exhausts"
            && std::find(path.begin(),path.end(),"corona") == path.end() && std::find(path.begin(),path.end(),"shadow") == path.end();
        const bool objectOnly = plate || sequence || path[0]=="carcols" || path[0]=="exhausts" || std::find(path.begin(),path.end(),"corona")!=path.end()
            || (path[0]=="lights" && std::find(path.begin(),path.end(),"shadow")!=path.end());
        Json edited=node;
        if (Color(edited,"##value",refs,refAllowed)) {
            if (objectOnly && !(sequence && edited.is_string() && refs.contains(edited.get<std::string>()))) { node=StudioSchema::ObjectColor(edited); }
            else node=std::move(edited);
            changed=true;
        }
    } else if (node.is_object() || node.is_array()) {
        const auto start=ImGui::GetCursorScreenPos();
        const float right=start.x+ImGui::GetContentRegionAvail().x;
        bool expanded = path.size()==1 || ImGui::TreeNodeEx("group",ImGuiTreeNodeFlags_FramePadding,"%s",label.c_str());
        if (remove) {
            const auto next=ImGui::GetCursorScreenPos();
            const float buttonX=std::max(start.x,right-ImGui::GetStyle().CellPadding.x-ImGui::GetFrameHeight());
            ImGui::SetCursorScreenPos(ImVec2(buttonX,start.y));
            *remove=StudioTheme::CloseButton(ImGui::GetFrameHeight()/30.0f);
            StudioTheme::Hint("Remove this setting or item.");
            ImGui::SetCursorScreenPos(next);
        }
        if (expanded) {
            std::string remove;
            size_t removeIndex = SIZE_MAX,index = 0;
            const auto fields=support.Fields(path,&root);
            index=0;
            for (auto it=node.begin();it!=node.end() && shown<2000;++it,++index) {
                auto name=node.is_object() ? it.key() : std::to_string(index);
                auto child=path; child.push_back(name); if (!support.Allows(child,&root)) continue;
                if (it->is_primitive() && !filter.PassFilter(name.c_str())) continue;
                path.push_back(name);
                bool erase=false;
                const bool edited=Edit(*it,path,root,filter,frames,audio,shown,support,&erase);
                if (erase) { if (node.is_object()) remove=name; else removeIndex=index; }
                changed |= edited;
                if (edited && name=="type" && *it=="rotator" && fields.contains("rotator") && !node.contains("rotator")) node["rotator"]=fields.at("rotator");
                path.pop_back();
                ImGui::PushID(name.c_str());
                const bool formats=(((path.size()==1 && path[0]=="neon" && name=="size") || (path[0]=="lights" && path.back()=="shadow" && name=="offset")) && !it->is_object())
                    || (name=="reference" && it->is_string()) || (name=="diffuse" && it->is_boolean());
                if (formats && ImGui::SmallButton("Format...")) ImGui::OpenPopup("override");
                if (ImGui::BeginPopup("override")) {
                    if (((path.size()==1 && path[0]=="neon" && name=="size") || (path[0]=="lights" && path.back()=="shadow" && name=="offset")) && !it->is_object()
                        && ImGui::MenuItem("Separate X / Y")) {
                        double x=0,y=0;
                        if (it->is_number()) { y=it->get<double>(); x=path[0]=="neon" ? y : 0; }
                        else if (it->is_array() && !it->empty()) { x=(*it)[0].get<double>(); if (it->size()>1) y=(*it)[1].get<double>(); }
                        *it={{"x",x},{"y",y}}; changed=true;
                    }
                    if (name=="reference" && it->is_string() && ImGui::MenuItem("Use multiple presets")) { *it=Json::array({*it}); changed=true; }
                    if (name=="diffuse" && it->is_boolean() && ImGui::MenuItem("Separate color / transparency")) { bool enabled=it->get<bool>(); *it={{"color",enabled},{"transparent",false}}; changed=true; }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            if (!remove.empty()) { node.erase(remove); changed=true; }
            if (removeIndex!=SIZE_MAX) { node.erase(node.begin()+removeIndex); changed=true; }
            if (node.is_array()) {
                if (node.size()<(path.back()=="offset" ? 2u : 1024u) && ImGui::Button("+ Add item")) { auto v=ArrayItem(path,node); if (!v.is_null()) { node.push_back(v); changed=true; } }
                if (path.back()=="pattern") { ImGui::SameLine(); if (ImGui::Button("+ Repeat block")) { node.push_back(Json::array({2,100,100})); changed=true; } }
            } else {
                if (ImGui::Button("+ Add setting")) ImGui::OpenPopup("add");
                if (ImGui::BeginPopup("add")) {
                    bool any=false;
                    static std::string customDummyName;
                    if (path.size()==1 && path[0]=="lights") {
                        ImGui::TextDisabled("Custom dummy name");
                        ImGui::SetNextItemWidth(-1);
                        ImGui::InputTextWithHint("##custom_dummy_name","e.g. indicator_lr_bg or fog_light7",&customDummyName);
                        std::string customKey;
                        for (const auto &frame : frames) {
                            auto frameKey=StudioSupport::ConfigNodeName(frame);
                            if (StudioSupport::NameEqualsInsensitive(frameKey,customDummyName)) { customKey=std::move(frameKey); break; }
                        }
                        auto customTypes=customKey.empty() ? std::vector<eMaterialType>{} : StudioSupport::DummyTypes(customKey,0,0,false,true);
                        auto customPath=path; customPath.emplace_back(customKey);
                        const bool canAdd=!customKey.empty() && !customTypes.empty()
                            && !StudioSupport::IsVanillaSideLightKey(customKey)
                            && !StudioSupport::HasKeyInsensitive(node,customKey)
                            && support.Allows(customPath,&root);
                        ImGui::BeginDisabled(!canAdd);
                        if (ImGui::Button("Add custom dummy") && canAdd) {
                            node[customKey]=Json::object();
                            customDummyName.clear(); changed=true; ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndDisabled();
                        ImGui::SameLine();
                        ImGui::TextDisabled("Must match a supported node in this DFF.");
                        any |= canAdd;
                    }
                    ImGui::Separator();
                    auto fields=support.Fields(path,&root);
                    std::set<std::string,StudioSupport::NameLessInsensitive> offeredNames,offeredLabels;
                    if (fields.is_object()) for (const auto &[name,v] : fields.items()) if (!node.contains(name)) {
                        if (path[0]=="lights" && StudioSupport::IsVanillaSideLightKey(name)) continue;
                        const auto label=Label(name);
                        if (StudioSupport::HasKeyInsensitive(node,name) || !offeredNames.insert(name).second
                            || !offeredLabels.insert(label).second) continue;
                        any=true;
                        ImGui::PushID(name.c_str());
                        if (ImGui::Selectable(label.c_str())) { StudioSchema::AddField(node,path,name,v); changed=true; ImGui::CloseCurrentPopup(); }
                        ImGui::PopID();
                    }
                    if (path.size()==1) for (const auto &frame : frames) {
                        auto name=path[0]=="lights" ? StudioSupport::ConfigNodeName(frame) : frame;
                        if (path[0]=="lights" && StudioSupport::IsVanillaSideLightKey(name)) continue;
                        if (StudioSupport::HasKeyInsensitive(node,name) || !offeredNames.insert(name).second) continue;
                        auto child=path; child.push_back(name);
                        if (StudioSchema::NodeFields(path[0],name).empty() || !support.Allows(child,&root)) continue;
                        const auto label=Label(name);
                        if (!offeredLabels.insert(label).second) continue;
                        any=true;
                        ImGui::PushID(name.c_str());
                        if (ImGui::Selectable(label.c_str())) { node[name]=Json::object(); changed=true; ImGui::CloseCurrentPopup(); }
                        ImGui::PopID();
                    }
                    static std::string name;
                    const bool palette = path[0]=="colors" || (path[0]=="sirens" && path.back()=="colors");
                    const bool states = path[0]=="sirens" && path.back()=="states";
                    const bool reference = path[0]=="sirens" && path.back()=="references";
                    const bool material = path[0]=="sirens" && ((path.size()==3 && path[1]=="states") || (path.size()==2 && path[1]!="states" && path[1]!="references"));
                    if (material) {
                        for (const auto &[id,flags] : support.entries.at("sirens")) if (!node.contains(id)) {
                            auto child=path; child.push_back(id); if (!support.Allows(child,&root)) continue;
                            any=true;
                            if (ImGui::Selectable(("Siren ID " + id).c_str())) { node[id]=Json::object(); changed=true; ImGui::CloseCurrentPopup(); }
                        }
                    }
                    if (palette || states || reference) {
                        ImGui::InputText("Name",&name);
                        if (!name.empty() && !node.contains(name) && ImGui::Button("Add")) {
                            node[name] = palette ? StudioSchema::Color() : Json::object();
                            changed=true; name.clear(); ImGui::CloseCurrentPopup();
                        }
                    } else if (!any) ImGui::TextDisabled("No missing supported settings. Node settings require an adapted, loaded model.");
                    ImGui::EndPopup();
                }
            }
            if (path.size()>1) ImGui::TreePop();
        }
    } else if (node.is_boolean() || (path[0]=="sirens" && (key=="state" || key=="imvehft") && node.is_number())) { bool v=node.is_boolean() ? node.get<bool>() : node.get<double>()!=0; if (StudioTheme::Toggle("##value",&v)) { node=v; changed=true; } }
    else if (path[0]=="metadata" && key=="minver" && node.is_number_integer()) {
        const int value=node.get<int>();
        const auto current=std::find_if(StudioSchema::MinimumVersions.begin(),StudioSchema::MinimumVersions.end(),[&](const auto &item) { return item.first==value; });
        const std::string label=current==StudioSchema::MinimumVersions.end() ? "Unsupported version" : std::string(current->second);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##value",label.c_str())) {
            for (const auto &[number,name] : StudioSchema::MinimumVersions) if (ImGui::Selectable(name.data(),value==number)) { node=number; changed=true; }
            ImGui::EndCombo();
        }
    }
    else if (path[0]=="sirens" && key=="paintjob" && node.is_number_integer()) {
        const int value=node.get<int>();
        const auto selected=value==-1 ? std::string("Any paintjob") : "Paintjob "+std::to_string(value);
        if (ImGui::BeginCombo("##value",selected.c_str())) {
            if (ImGui::Selectable("Any paintjob",value==-1)) { node=-1; changed=true; }
            for (unsigned i=0;i<support.remaps;++i) if (ImGui::Selectable(("Paintjob "+std::to_string(i)).c_str(),value==static_cast<int>(i))) { node=i; changed=true; }
            ImGui::EndCombo();
        }
        StudioTheme::Hint("Use every paintjob or restrict this siren state to an installed model remap.");
    }
    else if (node.is_number()) {
        double v=node.get<double>();
        const auto [min,max]=StudioSchema::NumericRange(path,node,root);
        const bool integer=node.is_number_integer();
        ImGui::SetNextItemWidth(-1);
        if (StudioTheme::Slider("##value",ImGuiDataType_Double,&v,min,max,integer ? "%.0f" : "%.3f")) { node=integer ? Json(static_cast<int64_t>(std::round(v))) : Json(v); changed=true; }
    } else if (node.is_string()) {
        auto choices=StudioSchema::Choices(path);
        if (path[0]=="sirens" && (key=="sound" || key=="audio")) { for (const auto &file : audio) choices.push_back(file); }
        if (path[0]=="sirens" && (key=="reference" || (path.size()>1 && path[path.size()-2]=="reference"))
            && root["sirens"].contains("references")) for (auto it=root["sirens"]["references"].begin();it!=root["sirens"]["references"].end();++it)
                if (it.key()!="colors") choices.push_back(it.key());
        if (!choices.empty()) {
            changed |= Combo(node,"##value",choices);
            if (key=="sound" || key=="audio" || key=="reference" || key=="texture" || (key=="type" && path.size()>1 && path[path.size()-2]=="shadow")) {
                if (key=="texture" || key=="type") StudioTheme::Hint("Bundled ME_TEXDB shadow textures. An empty light texture uses its normal texture.");
                else StudioTheme::Hint("Choose an installed audio file or reference preset; custom names remain editable.");
                if (ImGui::TreeNode("Custom name / path")) { changed |= ImGui::InputText("Name",&node.get_ref<std::string&>()); ImGui::TreePop(); }
            }
        } else changed |= ImGui::InputText("##value",&node.get_ref<std::string&>());
    } else ImGui::TextDisabled("%s: null",label.c_str());
    if (row) {
        if (remove) {
            ImGui::TableNextColumn();
            *remove=StudioTheme::CloseButton(ImGui::GetFrameHeight()/30.0f);
            StudioTheme::Hint("Remove this setting or item.");
        }
        ImGui::EndTable();
    }
    ImGui::PopID(); return changed;
}
}
