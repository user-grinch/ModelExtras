#pragma once
#include "studio_schema.h"
#include "utils/stringutil.h"
#include "../enums/materialtype.h"
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Inspection results contain names/flags only, never streaming-owned RW pointers.
namespace StudioSupport {

bool NameEqualsInsensitive(std::string_view a, std::string_view b);

struct NameLessInsensitive {
    bool operator()(std::string_view a, std::string_view b) const {
        const size_t count = std::min(a.size(), b.size());
        for (size_t i = 0; i < count; ++i) {
            const auto ac = std::tolower(static_cast<unsigned char>(a[i]));
            const auto bc = std::tolower(static_cast<unsigned char>(b[i]));
            if (ac != bc) return ac < bc;
        }
        return a.size() < b.size();
    }
};

int CompareNameInsensitive(std::string_view a, std::string_view b);
void DeduplicateNames(std::vector<std::string> &names);
bool IsVanillaSideLightKey(std::string_view name);
std::string ConfigNodeName(std::string_view name);
bool HasNameInsensitive(const std::vector<std::string> &names, std::string_view name);
bool HasKeyInsensitive(const StudioConfig::Json &object, std::string_view name);

// Forward to StringUtil for bounded integer conversion and prefix parsing.
inline std::optional<int> DigitsAfter(std::string_view name, std::string_view prefix) {
    return StringUtil::GetDigitsAfter(name, prefix);
}
inline std::optional<std::string> CharsAfter(std::string_view name, std::string_view prefix, size_t count) {
    return StringUtil::GetCharsAfterPrefix(name, prefix, count);
}

std::vector<eMaterialType> DummyTypes(std::string_view name, float x, float y, bool bike, bool headlights);

enum Capability : unsigned { Material = 1, Dummy = 2, Strobe = 4, Rotation = 8, Movement = 16, StandardSiren = 32, IVFSiren = 64 };

struct Model {
    bool inspected = false, limited = false, bike = false;
    unsigned paintSlots = 0, remaps = 0;
    std::map<std::string, std::map<std::string, unsigned>> entries;

    void Add(std::string section, std::string name, unsigned flags) { entries[section][name] |= flags; }

    unsigned Get(std::string_view section, std::string_view name) const {
        auto s = entries.find(std::string(section));
        if (s == entries.end()) return 0;
        auto n = s->second.find(std::string(name));
        return n == s->second.end() ? 0 : n->second;
    }

    bool Section(std::string_view name) const {
        auto s = std::string(name);
        if (s == "metadata" || s == "colors" || s == "neon" || s == "sound") return true;
        if (!inspected) return false;
        if (s == "carcols") return paintSlots != 0;
        auto it = entries.find(s);
        return it != entries.end() && !it->second.empty();
    }

    bool Allows(const std::vector<std::string> &p, const StudioConfig::Json *root = nullptr) const;
    StudioConfig::Json Fields(const std::vector<std::string> &p, const StudioConfig::Json *root = nullptr) const;
};

} // namespace StudioSupport
