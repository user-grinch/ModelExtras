#include "pch.h"
#include "studio_support.h"
#include <algorithm>
#include <charconv>

namespace StudioSupport {

bool NameEqualsInsensitive(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    }
    return true;
}

int CompareNameInsensitive(std::string_view a, std::string_view b) {
    NameLessInsensitive less;
    if (less(a, b)) return -1;
    if (less(b, a)) return 1;
    return 0;
}

void DeduplicateNames(std::vector<std::string> &names) {
    std::sort(names.begin(), names.end(), [](const auto &a, const auto &b) {
        const auto order = CompareNameInsensitive(a, b);
        return order == 0 ? a < b : order < 0;
    });
    names.erase(std::unique(names.begin(), names.end(), [](const auto &a, const auto &b) {
        return NameEqualsInsensitive(a, b);
    }), names.end());
}

bool IsVanillaSideLightKey(std::string_view name) {
    return NameEqualsInsensitive(name, "headlight_l") || NameEqualsInsensitive(name, "headlight_r")
        || NameEqualsInsensitive(name, "taillight_l") || NameEqualsInsensitive(name, "taillight_r");
}

std::string ConfigNodeName(std::string_view name) {
    return std::string(name.substr(0, name.find("_prm")));
}

bool HasNameInsensitive(const std::vector<std::string> &names, std::string_view name) {
    return std::any_of(names.begin(), names.end(), [&](const auto &candidate) { return NameEqualsInsensitive(candidate, name); });
}

bool HasKeyInsensitive(const StudioConfig::Json &object, std::string_view name) {
    if (!object.is_object()) return false;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (NameEqualsInsensitive(it.key(), name)) return true;
    }
    return false;
}

std::vector<eMaterialType> DummyTypes(std::string_view name, float x, float y, bool bike, bool headlights) {
    if (headlights) {
        if (name == "headlights" || name == "headlights2") {
            return bike ? std::vector<eMaterialType>{HeadLightLeft} : std::vector<eMaterialType>{HeadLightLeft, HeadLightRight};
        }
        if (name == "highbeam_l") return {HighBeamLeft};
        if (name == "highbeam_r") return {HighBeamRight};
        if (name == "highbeam") {
            return bike ? std::vector<eMaterialType>{HighBeamLeft} : std::vector<eMaterialType>{HighBeamLeft, HighBeamRight};
        }
    }
    if (name == "taillights" || name == "taillights2") {
        return bike ? std::vector<eMaterialType>{TailLightRight} : std::vector<eMaterialType>{TailLightLeft, TailLightRight};
    }
    const bool left = name.ends_with("_l") || name.find("_l_") != std::string_view::npos;
    const bool right = name.ends_with("_r") || name.find("_r_") != std::string_view::npos;
    if (name.starts_with("breakl") || name.starts_with("breaklight")) {
        if (left) return {BrakeLightLeft};
        if (right) return {BrakeLightRight};
        if (name.find("_m") != std::string_view::npos) return {(x < 0.0f) ? BrakeLightLeft : BrakeLightRight};
        return {};
    }
    if (name.starts_with("fogl") || name.starts_with("foglight")) {
        if (left) return {FogLightLeft};
        if (right) return {FogLightRight};
        return {};
    }
    if (name.starts_with("revl") || name.starts_with("reversingl")) {
        if (left) return {ReverseLightLeft};
        if (right) return {ReverseLightRight};
        return {};
    }
    if (name.starts_with("turnl") || name.starts_with("indicator")) {
        if (name.find("_lf") != std::string_view::npos) return {IndicatorLightLeftFront};
        if (name.find("_rf") != std::string_view::npos) return {IndicatorLightRightFront};
        if (name.find("_lr") != std::string_view::npos) return {IndicatorLightLeftRear};
        if (name.find("_rr") != std::string_view::npos) return {IndicatorLightRightRear};
        if (name.find("_lm") != std::string_view::npos) return {IndicatorLightLeftMiddle};
        if (name.find("_rm") != std::string_view::npos) return {IndicatorLightRightMiddle};
        return {};
    }
    for (auto [prefix, l, r] : {std::tuple{"sidelight_", SideLightLeft, SideLightRight}, {"sttlight_", STTLightLeft, STTLightRight}, {"nabrakelight_", NABrakeLightLeft, NABrakeLightRight}}) {
        if (auto d = CharsAfter(name, prefix, 1)) return {*d == "L" ? l : r};
    }
    if (name.starts_with("light_a")) return {AllDayLight};
    if (name.starts_with("light_d")) return {DayLight};
    if (name.starts_with("light_n")) return {NightLight};
    if (name.starts_with("light_") && !name.starts_with("light_em")) return {AllDayLight};
    if (name.starts_with("spotlight_light")) return {SpotLight};
    if (DigitsAfter(name, "strobe_light")) return {StrobeLight};
    return {};
}

bool Model::Allows(const std::vector<std::string> &p, const StudioConfig::Json *root) const {
    if (p.empty()) return true;
    auto s = p[0];
    if (!Section(s)) return false;
    if (p.size() == 1 || s == "metadata" || s == "colors" || s == "neon" || s == "sound") return true;
    if (s == "carcols") {
        if (p.size() < 4 || p[1] != "variations") return true;
        const char *slots[] = {"primary", "secondary", "tertiary", "quaternary"};
        for (unsigned i = 0; i < 4; ++i) {
            if (p[3] == slots[i]) return (paintSlots & (1u << i)) != 0;
        }
        return true;
    }
    if (s == "plate") return true;
    if (s == "sirens") {
        if (p[1] == "references") return true; // Presets can be shared by any supported ID.
        size_t id = p[1] == "states" ? 3 : 2;
        if (p.size() <= id || p[id] == "sound" || p[id] == "audio") return true;
        if (p[id] == "paintjob") return remaps > 0;
        int number = 0;
        auto parsed = std::from_chars(p[id].data(), p[id].data() + p[id].size(), number);
        unsigned flags = parsed.ec == std::errc() ? Get(s, std::to_string(number)) : 0;
        if (root && (flags & (StandardSiren | IVFSiren))) {
            bool ivf = root->contains("sirens") && root->at("sirens").is_object() && root->at("sirens").contains("imvehft")
                && root->at("sirens").at("imvehft").is_boolean() && root->at("sirens").at("imvehft").get<bool>();
            if (!(flags & (ivf ? IVFSiren : StandardSiren))) flags &= ~Material;
        }
        if (!(flags & (Material | Dummy))) return false;
        if (p.size() == id + 1) return true;
        const auto &field = p[id + 1];
        if (field == "size") return (flags & Dummy) != 0;
        if (field == "shadow") {
            if (p.size() == id + 2 || p[id + 2] == "angleoffset") return true;
            return (flags & Dummy) != 0;
        }
        if (field == "pointlight") return true;
        if (field == "diffuse" || field == "color_off") return (flags & Material) != 0;
        return true;
    }

    if (s == "lights" && p[1] == "inertia") {
        auto it = entries.find(s);
        return it != entries.end() && !it->second.empty();
    }
    unsigned flags = Get(s, p[1]);
    if (!flags) return false;
    if (p.size() == 2) return true;
    if (s == "lights" || s == "leds") {
        if (p[2] == "bulb" || p[2] == "preset") {
            const auto &k = p[1];
            bool isHeadlight = (k == "headlights" || k == "headlight_l" || k == "headlight_r" ||
                                k.starts_with("highbeam") || k.starts_with("headlight"));
            return isHeadlight && ((flags & (Dummy | Material)) != 0);
        }
        if (p[2] == "material" || p[2] == "color" || p[2] == "color_off") return (flags & Material) != 0;
        if (p[2] == "corona" || p[2] == "shadow" || p[2] == "pointlight") return (flags & Dummy) != 0;
        if (p[2] == "strobedelay") return (flags & Strobe) != 0;
        return p[2] == "inertia";
    }
    if (s == "rollback_bed") {
        if (p[2] == "target_rot" || p[2] == "rot_speed") return (flags & Rotation) != 0;
        if (p[2] == "target_move" || p[2] == "move_speed") return (flags & Movement) != 0;
    }
    return true;
}

StudioConfig::Json Model::Fields(const std::vector<std::string> &p, const StudioConfig::Json *root) const {
    auto fields = StudioSchema::Fields(p);
    if (!fields.is_object()) return fields;
    auto child = p; child.emplace_back();
    for (auto it = fields.begin(); it != fields.end();) {
        child.back() = it.key();
        if (!Allows(child, root)) it = fields.erase(it);
        else ++it;
    }
    return fields;
}

} // namespace StudioSupport
