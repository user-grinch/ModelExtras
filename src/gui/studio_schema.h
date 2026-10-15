#pragma once
#include "studio_config.h"
#include <array>
#include <charconv>
#include <cstdio>
#include <vector>

// Editor vocabulary follows the actual JSONC consumers. Defaults are suggestions,
// never inserted merely by viewing a page (DFF/INI defaults may be different).
namespace StudioSchema {
using StudioConfig::Json;
inline constexpr std::array<std::pair<int,std::string_view>,6> MinimumVersions = {{{10600,"v1.6"},{10700,"v1.7"},{20000,"v2.0"},{20100,"v2.1"},{30000,"v3.0"},{30100,"v3.1"}}};
inline const Json &Color() { static const Json v = {{"red",255},{"green",255},{"blue",255},{"alpha",255}}; return v; }
inline const Json &Material() { static const Json v = {{"color",Color()},{"color_off",{{"red",16},{"green",16},{"blue",16},{"alpha",255}}}}; return v; }
inline const Json &Light() {
    static const Json v = {{"material",Material()},
        {"inertia",0.0},{"strobedelay",1000},
        {"corona",{{"size",0.4},{"type","non-directional"},{"color",Color()}}},
        {"shadow",{{"size",1.0},{"texture","headlight_short"},{"rotationchecks",true},{"offset",{{"x",0.0},{"y",0.0}}},{"color",Color()}}}};
    return v;
}
inline const Json &SirenMaterial() {
    static const Json v = {{"size",0.6},{"radius",180.0},{"color",Color()},{"color_off",Color()},
        {"state",false},{"diffuse",{{"color",false},{"transparent",false}}},
        {"pattern",Json::array({100,100})},{"colors",Json::array({Json::array({1000,Color()})})},
        {"type","directional"},{"delay",0},{"inertia",0.0},{"imvehft",false},{"reference",""},
        {"rotator",{{"direction","clockwise"},{"type","linear"},{"time",1000},{"offset",0.0},{"radius",360.0}}},
        {"shadow",{{"size",0.0},{"type","round"},{"offset",0.0},{"angleoffset",0.0}}}};
    return v;
}
inline void AddField(Json &node, const std::vector<std::string> &path, const std::string &name, const Json &value) {
    if (name=="states" && path.size()==1 && path[0]=="sirens" && !node.contains("states")) {
        Json states=Json::object();
        for (auto it=node.begin();it!=node.end();) {
            if (it.key()!="references" && it.key()!="imvehft") { states[it.key()]=it.value(); it=node.erase(it); }
            else ++it;
        }
        node["states"]=std::move(states);
    } else node[name]=value.is_object()
        ? (path.size()==1 && path[0]=="neon" && (name=="size" || name=="offset") ? value : Json::object())
        : value;
}
inline bool Prefix(std::string_view name, std::string_view prefix) { return name.starts_with(prefix); }
inline const char *Help(std::string_view section) {
    if (section=="neon") return "Underbody light: fixed color, rainbow, vehicle colors or one paint slot. Requires Neon enabled in Settings; vehicle lights are required by default but can be disconnected.";
    if (section=="lights") return "Material colors and existing light dummies. Specific light settings override group settings. Corona/shadow colors use RGBA; adding a shadow override enables its shadow.";
    if (section=="sirens") return "States, existing siren IDs, flash timings, color sequences, rotators, audio and reusable presets. Requires an adapted siren model and Siren Lights enabled at game startup.";
    if (section=="colors") return "Reusable color names for light materials. Siren colors have a separate palette under Sirens > References > Colors.";
    if (section=="carcols") return "RGB paint palette and variations. Paint slots select palette indices starting at zero. Requires Carcols enabled; unavailable in SA-MP.";
    if (section=="sound") return "Per-model door chime and brake-pad sound overrides. Other sounds use the global Settings controls.";
    if (section=="plate") return "Day/night plate material colors. Requires HD License Plate enabled at startup. This checkout does not read JSONC plate text or font styling.";
    if (section=="leds") return "Dashboard indicator material colors. Only the model's adapted LED materials will respond.";
    if (section=="metadata") return "Author, description, creation date and required mod version. These describe the model; they do not create vehicle effects.";
    return "Configure the model's existing adapted nodes. Add setting lists recognised loaded nodes; no DFF nodes are created. The corresponding feature must be enabled in Settings.";
}
inline Json NodeFields(std::string_view section, std::string_view name) {
    const auto &t = StudioConfig::Templates();
    if (section == "lights") return Light();
    if (section == "leds") return Json{{"material",Material()}};
    if (section == "exhausts" && Prefix(name,"x_exhaust")) return t.at("exhausts").begin().value();
    if (section == "roofs" && (Prefix(name,"x_convertible_roof") || Prefix(name,"x_convertible_boot"))) return t.at("roofs").begin().value();
    if (section == "spoilers" && Prefix(name,"movspoiler")) return t.at("spoilers").begin().value();
    if (section == "clocks" && Prefix(name,"x_dclock")) return t.at("clocks").begin().value();
    if (section == "doors") {
        for (auto *prefix : {"x_rd_lf","x_rd_rf","x_rd_lr","x_rd_rr","x_rd_boot","x_rd_bonnet"}) if (Prefix(name,prefix)) return t.at("doors").at("x_rd_lf");
        for (auto *prefix : {"x_sd_lf","x_sd_rf","x_sd_lr","x_sd_rr","dvan_l","dvan_r","dmbus_l","dmbus_r"}) if (Prefix(name,prefix)) return t.at("doors").at("x_sd_lf");
    }
    if (section == "gauges") {
        if (Prefix(name,"x_rpm") || Prefix(name,"fc_rpm") || Prefix(name,"tahook")) return t.at("gauges").at("x_rpm");
        if (Prefix(name,"x_sm") || Prefix(name,"fc_sm") || Prefix(name,"speedook")) return t.at("gauges").at("x_sm");
        if (Prefix(name,"x_tm")) return t.at("gauges").at("x_tm");
        if (Prefix(name,"x_odometer") || Prefix(name,"fc_om")) return t.at("gauges").at("x_odometer");
        if (Prefix(name,"x_gauge_fixed") || name == "x_gasmeter" || name == "x_gm" || name == "petrolok") return t.at("gauges").at("x_gasmeter");
    }
    return Json::object();
}
inline const Json &Sections() {
    static const Json v = [] {
        Json v = StudioConfig::Templates();
        v["metadata"].update(Json{{"creationtime",""}});
        v["lights"] = {{"inertia",0.0}};
        for (const auto *key : {"headlights","taillights","brakelights","reverselights","indicators","foglights","sidelights","daylights","nightlights","alldaylights","spotlights","strobelights","highbeam",
             "highbeam_l","highbeam_r","sttlight_l","sttlight_r","brakelight_l","brakelight_r","reverselight_l","reverselight_r",
             "indicator_lf","indicator_rf","indicator_lr","indicator_rr","indicator_lm","indicator_rm","foglight_l","foglight_r","sidelight_l","sidelight_r","daylight","nightlight","alldaylight","spotlight","strobelight",
             "leds"}) v["lights"][key] = Json::object();
        v["leds"] = Json::object();
        for (const auto *key : {"engine_on","engine_broken","fog_light","high_beam","low_beam","indicator_left","indicator_right","siren","boot_open","bonnet_open","door_open","roof_open","leds"}) {
            v["leds"][key] = Json::object(); v["lights"][key] = Json::object();
        }
        for (auto *section : {"exhausts","roofs","spoilers","doors","gauges","clocks"}) v[section] = Json::object();
        v["lights"]["plate"] = Json::object();
        for (const auto &[key,value] : v["lights"].items()) if (key!="inertia" && key!="plate") v["leds"][key]=Json::object();
        v["sirens"] = {{"imvehft",false},{"states",Json::object()},{"references",{{"colors",Json::object()}}}};
        return v;
    }(); return v;
}
inline Json Fields(const std::vector<std::string> &p) {
    if (p.empty()) return Sections();
    auto root = p[0];
    if (!Sections().contains(root)) return Json::object();
    Json v = Sections().at(root);
    if (root == "sirens") {
        if (p.size() == 1) return v;
        if (p[1] == "references") {
            if (p.size() == 2) return {{"colors",Json::object()}};
            if (p[2] == "colors") return Json::object();
            v = SirenMaterial();
            for (size_t i = 3; i < p.size(); ++i) { if (!v.contains(p[i])) return Json::object(); v = Json(v.at(p[i])); }
            return v;
        }
        size_t stateDepth = p[1] == "states" ? 3 : 2;
        if (p.size() < stateDepth) return Json::object();
        if (p.size() == stateDepth) return {{"paintjob",-1},{"sound",""},{"audio",""}};
        v = SirenMaterial();
        for (size_t i = stateDepth + 1; i < p.size(); ++i) { if (!v.is_object() || !v.contains(p[i])) return Json::object(); v = Json(v.at(p[i])); }
        return v;
    }
    if (p.size() >= 2 && (root == "lights" || root == "leds" || root == "exhausts" || root == "roofs" || root == "spoilers" || root == "doors" || root == "gauges" || root == "clocks")) {
        if (root == "lights" && p[1] == "plate") v = Sections().at("plate");
        else v = NodeFields(root,p[1]);
        for (size_t i = 2; i < p.size(); ++i) { if (!v.is_object() || !v.contains(p[i])) return Json::object(); v = Json(v.at(p[i])); }
        return v;
    }
    for (size_t i = 1; i < p.size(); ++i) {
        if (v.is_array()) { if (v.empty()) return Json::object(); v = Json(v[0]); }
        else if (v.is_object() && v.contains(p[i])) v = Json(v.at(p[i]));
        else return Json::object();
    }
    return v;
}
inline std::pair<double,double> NumericRange(const std::vector<std::string> &path, const Json &node, const Json &root) {
    const auto &key=path.back();
    const double v=node.get<double>();
    double min=0,max=std::max(10.0,v);
    // Position/direction can be signed. Progress speeds, sizes and counts cannot.
    const bool offset=std::find(path.begin(),path.end(),"offset")!=path.end();
    const bool signedValue=offset || key.find("angle")!=key.npos || key=="rotation" || key=="maxrotation" || key=="target_rot" || key=="target_move"
        || (path[0]=="doors" && (key=="mul" || key=="movmul" || key=="popout"))
        || (path[0]=="neon" && (key=="speed" || key=="offset"))
        || (path[0]=="sirens" && path.size()>1 && path[path.size()-2]=="rotator" && key=="radius");
    if (signedValue) { const double limit=key.find("angle")!=key.npos || key=="radius" || key.find("rot")!=key.npos ? 360.0 : 10.0; min=std::min(-limit,v); max=std::max(limit,max); }
    if (key.find("rpm")!=key.npos) max=std::max(20000.0,v);
    if (key.find("speed")!=key.npos || key=="maxturbo") max=std::max(500.0,v);
    if (key=="time" || key=="delay" || key=="strobedelay" || key=="minver" || (path[0]=="sirens" && (std::find(path.begin(),path.end(),"pattern")!=path.end() || (path.size()>2 && path[path.size()-3]=="colors" && key=="0")))) max=std::max(60000.0,v);
    if (key=="radius" || key.find("rotation")!=key.npos || key.find("angle")!=key.npos) max=std::max(360.0,v);
    if (path[0]=="sirens" && std::find(path.begin(),path.end(),"pattern")!=path.end()) { min=0; if (path.size()>=2 && path[path.size()-2]!="pattern" && key=="0") { min=1; max=100; } }
    if (path[0]=="sirens" && path.size()>2 && path[path.size()-3]=="colors" && key=="0") min=1;
    if (key=="time" || key=="strobedelay" || key=="maxrpm" || key=="maxspeed" || key=="maxturbo") min=1;
    if (path[0]=="carcols" && std::find(path.begin(),path.end(),"variations")!=path.end() && root["carcols"].contains("colors"))
        max=static_cast<double>(root["carcols"]["colors"].size()>0 ? root["carcols"]["colors"].size()-1 : 0);
    return {min,max};
}
inline bool IsColor(const Json &v, const std::vector<std::string> &p) {
    if (p.empty() || v.is_boolean()) return false;
    auto k = p.back();
    if (p.size()==1 && k=="neon"
        && (v.is_string() || v.is_array() || (v.is_object() && (v.contains("red") || v.contains("r"))))) return true;
    if (k == "color" || k == "color_off") return true;
    if (p[0] == "colors" && p.size() == 2) return true;
    if (p[0] == "carcols" && p.size() == 3 && p[1] == "colors") return true;
    if (p[0] == "sirens" && p.size() == 4 && p[1] == "references" && p[2] == "colors") return true;
    return p[0] == "sirens" && p.size() >= 3 && p[p.size()-3] == "colors" && k == "1";
}
inline bool DecodeColor(const Json &v, float *rgba, const Json &refs, unsigned depth = 0) {
    if (depth > 32) return false;
    std::array<int,4> c{255,255,255,255};
    if (v.is_string()) {
        auto s = v.get<std::string>();
        if (refs.is_object() && refs.contains(s)) return DecodeColor(refs.at(s),rgba,refs,depth+1);
        if (s.starts_with('#')) s.erase(0,1); else if (s.starts_with("0x") || s.starts_with("0X")) s.erase(0,2);
        if (s.size() != 3 && s.size() != 4 && s.size() != 6 && s.size() != 8) return false;
        uint32_t n = 0; auto result = std::from_chars(s.data(),s.data()+s.size(),n,16);
        if (result.ec != std::errc() || result.ptr != s.data()+s.size()) return false;
        const bool shortHex = s.size() < 5, alpha = s.size() == 4 || s.size() == 8;
        for (int i = 0; i < (alpha ? 4 : 3); ++i) c[i] = (n >> ((alpha ? 3-i : 2-i) * (shortHex ? 4 : 8))) & (shortHex ? 15 : 255);
        if (shortHex) for (int i = 0; i < (alpha ? 4 : 3); ++i) c[i] *= 17;
    } else if (v.is_array() && v.size() >= 3) {
        for (size_t i = 0; i < std::min<size_t>(v.size(),4); ++i) { if (!v[i].is_number()) return false; c[i] = v[i].get<int>(); }
    } else if (v.is_object()) {
        const char *longKeys[] = {"red","green","blue","alpha"}, *shortKeys[] = {"r","g","b","a"};
        for (int i = 0; i < 4; ++i) { auto it = v.find(longKeys[i]); if (it == v.end()) it = v.find(shortKeys[i]); if (it != v.end()) { if (!it->is_number()) return false; c[i] = it->get<int>(); } }
    } else return false;
    for (int i = 0; i < 4; ++i) rgba[i] = std::clamp(c[i],0,255)/255.f;
    return true;
}
inline void EncodeColor(Json &v, const float *rgba) {
    int c[4]; for (int i=0;i<4;++i) c[i] = static_cast<int>(std::round(std::clamp(rgba[i],0.f,1.f)*255));
    if (v.is_array()) { for (size_t i=0;i<std::min<size_t>(v.size(),4);++i) v[i]=c[i]; if (v.size()==3 && c[3]!=255) v.push_back(c[3]); }
    else if (v.is_object()) {
        const char *l[] = {"red","green","blue","alpha"}, *s[] = {"r","g","b","a"};
        bool shortKeys = v.contains("r") && !v.contains("red");
        for (int i=0;i<4;++i) { if (i==3 && !v.contains("alpha") && !v.contains("a") && c[i]==255) continue; v[v.contains(l[i]) ? l[i] : v.contains(s[i]) ? s[i] : shortKeys ? s[i] : l[i]]=c[i]; }
    } else { char hex[10]; std::snprintf(hex,sizeof(hex),"#%02X%02X%02X%02X",c[0],c[1],c[2],c[3]); v=hex; }
}
// Strict consumers require long-key RGBA, while unknown per-color metadata survives.
inline Json ObjectColor(const Json &value) {
    float c[4]{1,1,1,1}; DecodeColor(value,c,Json::object());
    Json result=value.is_object() ? value : Json::object();
    for (auto *key : {"r","g","b","a"}) result.erase(key);
    result.update(Color()); EncodeColor(result,c);
    return result;
}
inline std::vector<std::string_view> Choices(const std::vector<std::string> &p) {
    if (p.empty()) return {};
    auto k = p.back();
    if (p[0] == "neon" && k == "mode") return {"static","rainbow","vehicle","primary","secondary","tertiary","quaternary"};
    if (k == "direction" && p[0] == "sirens") return {"clockwise","counter-clockwise","switch"};
    if (k == "type" && p.size()>1 && p[p.size()-2] == "rotator") return {"linear","ease"};
    if (k == "texture" || (k == "type" && p.size()>1 && p[p.size()-2] == "shadow")) {
        std::vector<std::string_view> choices={"round","headlight_short","headlight_long","foglight","taillight","taillight_bike","indicator","reverse","oval","neon","spotlight","tightfocused","narrow","pointlight","arealight","bollard","comet","cylindernarrow","defined","defineddiffuse","defineddiffusespot","definedspot","jellyfish","mediumscatter","overhead","parallelbeam","pear","rounddiffuse","scatterlight","softarrow","softdisplay","star","starfocused","threelobeumbrella","threelobevee","toppost","trapezoid","umbrella","vee","veeup","xarrow","xarrowdiffuse","xarrowsoft"};
        if (k=="texture") choices.insert(choices.begin(),"");
        return choices;
    }
    if (k == "type" && (p[0] == "sirens" || (p.size()>1 && p[p.size()-2] == "corona"))) return {"directional","inversed-directional","non-directional","rotator"};
    return {};
}
}
