#pragma once
#include "studio_schema.h"
#include <map>
#include <optional>
#include <tuple>
#include <cctype>
#include "../enums/materialtype.h"

// Inspection results contain names/flags only, never streaming-owned RW pointers.
namespace StudioSupport {
inline bool NameEqualsInsensitive(std::string_view a, std::string_view b) {
    if (a.size()!=b.size()) return false;
    for (size_t i=0;i<a.size();++i)
        if (std::tolower(static_cast<unsigned char>(a[i]))!=std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
struct NameLessInsensitive {
    bool operator()(std::string_view a,std::string_view b) const {
        const size_t count=std::min(a.size(),b.size());
        for (size_t i=0;i<count;++i) {
            const auto ac=std::tolower(static_cast<unsigned char>(a[i]));
            const auto bc=std::tolower(static_cast<unsigned char>(b[i]));
            if (ac!=bc) return ac<bc;
        }
        return a.size()<b.size();
    }
};
inline int CompareNameInsensitive(std::string_view a,std::string_view b) {
    NameLessInsensitive less;
    if (less(a,b)) return -1;
    if (less(b,a)) return 1;
    return 0;
}
inline void DeduplicateNames(std::vector<std::string> &names) {
    std::sort(names.begin(),names.end(),[](const auto &a,const auto &b) {
        const auto order=CompareNameInsensitive(a,b);
        return order==0 ? a<b : order<0;
    });
    names.erase(std::unique(names.begin(),names.end(),[](const auto &a,const auto &b) {
        return NameEqualsInsensitive(a,b);
    }),names.end());
}
inline bool IsVanillaSideLightKey(std::string_view name) {
    return NameEqualsInsensitive(name,"headlight_l") || NameEqualsInsensitive(name,"headlight_r")
        || NameEqualsInsensitive(name,"taillight_l") || NameEqualsInsensitive(name,"taillight_r");
}
inline std::string ConfigNodeName(std::string_view name) {
    return std::string(name.substr(0,name.find("_prm")));
}
inline bool HasNameInsensitive(const std::vector<std::string> &names,std::string_view name) {
    return std::any_of(names.begin(),names.end(),[&](const auto &candidate) { return NameEqualsInsensitive(candidate,name); });
}
inline bool HasKeyInsensitive(const StudioConfig::Json &object,std::string_view name) {
    if (!object.is_object()) return false;
    for (auto it=object.begin();it!=object.end();++it)
        if (NameEqualsInsensitive(it.key(),name)) return true;
    return false;
}
// Match the runtime name readers, with bounded integer conversion for inspection.
inline std::optional<int> DigitsAfter(std::string_view name, std::string_view prefix) {
    if (!name.starts_with(prefix)) return {};
    name.remove_prefix(prefix.size());
    if (!name.empty() && (name.front()=='_' || name.front()=='-')) name.remove_prefix(1);
    int number=0; auto parsed=std::from_chars(name.data(),name.data()+name.size(),number);
    if (name.empty() || name.front()<'0' || name.front()>'9' || parsed.ec!=std::errc()) return {};
    return number;
}
inline std::optional<std::string> CharsAfter(std::string_view name, std::string_view prefix, size_t count) {
    if (!name.starts_with(prefix)) return {};
    name.remove_prefix(prefix.size());
    if (!name.empty() && (name.front()=='_' || name.front()=='-')) name.remove_prefix(1);
    if (name.size()<count) return {};
    std::string result(name.substr(0,count));
    for (auto &c : result) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return result;
}
inline std::vector<eMaterialType> DummyTypes(std::string_view name, float x, float y, bool bike, bool headlights) {
    if (headlights) {
        if (name=="headlights" || name=="headlights2" || name=="headlight") return bike ? std::vector<eMaterialType>{HeadLightLeft} : std::vector<eMaterialType>{HeadLightLeft,HeadLightRight};
        if (name=="headlights_l" || name=="headlight_l") return {HeadLightLeft};
        if (name=="headlights_r" || name=="headlight_r") return {HeadLightRight};
        if (name=="highbeam_l" || name=="highbeams_l") return {HighBeamLeft};
        if (name=="highbeam_r" || name=="highbeams_r") return {HighBeamRight};
        if (name=="highbeam" || name=="highbeams") return bike ? std::vector<eMaterialType>{HighBeamLeft} : std::vector<eMaterialType>{HighBeamLeft,HighBeamRight};
    }
    if (name=="taillights" || name=="taillights2" || name=="taillight") return bike ? std::vector<eMaterialType>{TailLightRight} : std::vector<eMaterialType>{TailLightLeft,TailLightRight};
    if (name=="taillights_l" || name=="taillight_l") return {TailLightLeft};
    if (name=="taillights_r" || name=="taillight_r") return {TailLightRight};
    const bool left = name.ends_with("_l") || name.find("_l_") != name.npos || name.find("left") != name.npos;
    const bool right = name.ends_with("_r") || name.find("_r_") != name.npos || name.find("right") != name.npos;
    if ((name.starts_with("breakl") || name.starts_with("brakel") || name.starts_with("brake_") || name.starts_with("break_")) &&
        name.find("disc") == name.npos && name.find("caliper") == name.npos) {
        return {(left || (!right && x < -0.05f)) ? BrakeLightLeft : BrakeLightRight};
    }
    if (name.starts_with("fogl") || name.starts_with("fog_")) {
        return {(left || (!right && x < 0.0f)) ? FogLightLeft : FogLightRight};
    }
    if (name.starts_with("revers") || name.starts_with("revl")) return {(left || (!right && x < 0.0f)) ? ReverseLightLeft : ReverseLightRight};
    if (name.starts_with("turnl") || name.starts_with("indicator")) {
        if (name.find("_lf") != name.npos || name.find("_fl") != name.npos) return {IndicatorLightLeftFront};
        if (name.find("_rf") != name.npos || name.find("_fr") != name.npos) return {IndicatorLightRightFront};
        if (name.find("_lr") != name.npos || name.find("_rl") != name.npos) return {IndicatorLightLeftRear};
        if (name.find("_rr") != name.npos) return {IndicatorLightRightRear};
        if (name.find("_lm") != name.npos || name.find("_ml") != name.npos) return {IndicatorLightLeftMiddle};
        if (name.find("_rm") != name.npos || name.find("_mr") != name.npos) return {IndicatorLightRightMiddle};

        const bool l = (left && !right) ? true : (!left && right) ? false : (x < -0.05f);
        if (name.find("_m") != name.npos || name.find("side") != name.npos || name.find("mid") != name.npos) return {l ? IndicatorLightLeftMiddle : IndicatorLightRightMiddle};
        if (name.find("rear") != name.npos || name.find("back") != name.npos) return {l ? IndicatorLightLeftRear : IndicatorLightRightRear};
        if (name.find("front") != name.npos || name.find("_f") != name.npos) return {l ? IndicatorLightLeftFront : IndicatorLightRightFront};
        return {l ? (y >= 0.0f ? IndicatorLightLeftFront : IndicatorLightLeftRear) : (y >= 0.0f ? IndicatorLightRightFront : IndicatorLightRightRear)};
    }
    for (auto [prefix,l,r] : {std::tuple{"sidelight_",SideLightLeft,SideLightRight}, {"sttlight_",STTLightLeft,STTLightRight}, {"nabrakelight_",NABrakeLightLeft,NABrakeLightRight}})
        if (auto d=CharsAfter(name,prefix,1)) return {*d=="L" ? l : r};
    if (name.starts_with("light_a")) return {AllDayLight};
    if (name.starts_with("light_d")) return {DayLight};
    if (name.starts_with("light_n")) return {NightLight};
    if (name.starts_with("light_") && !name.starts_with("light_em")) return {AllDayLight};
    if (name.starts_with("spotlight_light")) return {SpotLight};
    if (DigitsAfter(name,"strobe_light")) return {StrobeLight};
    return {};
}
enum Capability : unsigned { Material=1, Dummy=2, Strobe=4, Rotation=8, Movement=16, StandardSiren=32, IVFSiren=64 };
struct Model {
    bool inspected=false, limited=false, bike=false;
    unsigned paintSlots=0, remaps=0;
    std::map<std::string,std::map<std::string,unsigned>> entries;
    void Add(std::string section, std::string name, unsigned flags) { entries[section][name] |= flags; }
    unsigned Get(std::string_view section, std::string_view name) const {
        auto s=entries.find(std::string(section));
        if (s==entries.end()) return 0;
        auto n=s->second.find(std::string(name));
        return n==s->second.end() ? 0 : n->second;
    }
    bool Section(std::string_view name) const {
        auto s=std::string(name);
        if (s=="metadata" || s=="colors" || s=="neon" || s=="sound") return true;
        if (!inspected) return false;
        if (s=="carcols") return paintSlots!=0;

        auto it=entries.find(s); return it!=entries.end() && !it->second.empty();
    }
    bool Allows(const std::vector<std::string> &p, const StudioConfig::Json *root=nullptr) const {
        if (p.empty()) return true;
        auto s=p[0];
        if (!Section(s)) return false;
        if (p.size()==1 || s=="metadata" || s=="colors" || s=="neon" || s=="sound") return true;
        if (s=="carcols") {
            if (p.size()<4 || p[1]!="variations") return true;
            const char *slots[]={"primary","secondary","tertiary","quaternary"};
            for (unsigned i=0;i<4;++i) if (p[3]==slots[i]) return (paintSlots & (1u<<i))!=0;
            return true;
        }
        if (s=="plate") return true;
        if (s=="sirens") {
            if (p[1]=="references") return true; // Presets can be shared by any supported ID.
            size_t id=p[1]=="states" ? 3 : 2;
            if (p.size()<=id || p[id]=="sound" || p[id]=="audio") return true;
            if (p[id]=="paintjob") return remaps>0;
            int number=0; auto parsed=std::from_chars(p[id].data(),p[id].data()+p[id].size(),number);
            unsigned flags=parsed.ec==std::errc() ? Get(s,std::to_string(number)) : 0;
            if (root && (flags&(StandardSiren|IVFSiren))) {
                bool ivf=root->contains("sirens") && root->at("sirens").is_object() && root->at("sirens").contains("imvehft")
                    && root->at("sirens").at("imvehft").is_boolean() && root->at("sirens").at("imvehft").get<bool>();
                if (!(flags & (ivf ? IVFSiren : StandardSiren))) flags &= ~Material;
            }
            if (!(flags&(Material|Dummy))) return false;
            if (p.size()==id+1) return true;
            const auto &field=p[id+1];
            if (field=="size") return (flags&Dummy)!=0;
            if (field=="shadow") {
                if (p.size()==id+2 || p[id+2]=="angleoffset") return true;
                return (flags&Dummy)!=0;
            }
            if (field=="pointlight") return true;
            if (field=="diffuse" || field=="color_off") return (flags&Material)!=0;
            return true;
        }

        if (s=="lights" && p[1]=="inertia") { auto it=entries.find(s); return it!=entries.end() && !it->second.empty(); }
        unsigned flags=Get(s,p[1]);
        if (!flags) return false;
        if (p.size()==2) return true;
        if (s=="lights" || s=="leds") {
            if (p[2]=="material" || p[2]=="color" || p[2]=="color_off") return (flags&Material)!=0;
            if (p[2]=="corona" || p[2]=="shadow" || p[2]=="pointlight") return (flags&Dummy)!=0;
            if (p[2]=="strobedelay") return (flags&Strobe)!=0;
            return p[2]=="inertia";
        }
        if (s=="rollback_bed") {
            if (p[2]=="target_rot" || p[2]=="rot_speed") return (flags&Rotation)!=0;
            if (p[2]=="target_move" || p[2]=="move_speed") return (flags&Movement)!=0;
        }
        return true;
    }
    StudioConfig::Json Fields(const std::vector<std::string> &p, const StudioConfig::Json *root=nullptr) const {
        auto fields=StudioSchema::Fields(p);
        if (!fields.is_object()) return fields;
        auto child=p; child.emplace_back();
        for (auto it=fields.begin();it!=fields.end();) {
            child.back()=it.key();
            if (!Allows(child,root)) it=fields.erase(it); else ++it;
        }
        return fields;
    }
};
}
