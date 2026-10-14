#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <string_view>

namespace StudioConfig {
using Json = nlohmann::json;
using IniChanges = std::map<std::pair<std::string, std::string>, std::string>;

inline std::string ModelList(const std::string &text, bool wrap = false) {
    std::string result;
    size_t count = 0;
    for (size_t i = 0; i < text.size();) {
        if (text[i] == ',' || text[i] == '"' || text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n') { ++i; continue; }
        int model = 0;
        const size_t first = i;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            model = model * 10 + (text[i++] - '0');
            if (model > 19999) throw std::runtime_error("Model list IDs must be 1..19999.");
        }
        if (i == first || model == 0) throw std::runtime_error("Model lists accept numeric IDs separated by commas or spaces.");
        if (count) result += wrap && count % 8 == 0 ? ",\n" : ", ";
        result += std::to_string(model);
        if (++count > 1024) throw std::runtime_error("Model list limit: 1024 IDs.");
    }
    return wrap ? result : "\"" + result + "\"";
}

inline Json Parse(const std::string &text) {
    if (text.size() > 1024 * 1024) throw std::runtime_error("Studio config limit: 1 MiB");
    return Json::parse(text, [](int depth, Json::parse_event_t, Json &) {
        if (depth > 32) throw std::runtime_error("Config nesting limit: 32");
        return true;
    }, true, true);
}

inline void SetField(Json &root, const std::string &path, const Json &value) {
    if (path.empty() || path[0] != '/' || path.size() > 4096) throw std::runtime_error("Use a JSON pointer beginning with / (up to 4096 bytes)");
    // Validate escapes; walk explicitly so a huge array index cannot allocate gigabytes.
    const Json::json_pointer validated(path);
    Json *node = &root;
    unsigned depth = 0;
    for (size_t start = 1;;) {
        if (++depth > 32) throw std::runtime_error("Config nesting limit: 32");
        auto end = path.find('/', start);
        auto token = path.substr(start, end == path.npos ? path.size() - start : end - start);
        for (size_t i = 0; i < token.size(); ++i) if (token[i] == '~') { token.replace(i, 2, token[i + 1] == '1' ? "/" : "~"); }
        if (node->is_null()) *node = Json::object();
        if (node->is_object()) node = &(*node)[token];
        else if (node->is_array()) {
            size_t index = node->size();
            if (token != "-") {
                if (token.empty() || token.find_first_not_of("0123456789") != token.npos) throw std::runtime_error("Array index must be numeric or -");
                const auto parsed = std::stoull(token);
                if (parsed > node->size()) throw std::runtime_error("Array index must exist or append one element");
                index = static_cast<size_t>(parsed);
            }
            if (index > node->size() || (index == node->size() && node->size() >= 4096)) throw std::runtime_error("Array index must exist or append one element (limit: 4096)");
            if (index == node->size()) node->push_back(nullptr);
            node = &(*node)[index];
        } else throw std::runtime_error("Path parent must be an object or array");
        if (end == path.npos) { *node = value; return; }
        start = end + 1;
    }
}

inline std::string Trim(std::string_view value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == value.npos) return {};
    return std::string(value.substr(start, value.find_last_not_of(" \t\r\n") - start + 1));
}

inline void ValidateIniValue(const std::string &value, bool number, bool text) {
    if (value.find_first_of("\r\n") != value.npos) throw std::runtime_error("INI values must be a single line");
    if (number && !std::isfinite(std::stod(value))) throw std::runtime_error("INI number must be finite");
    const auto trimmed = Trim(value);
    if (text && !trimmed.empty() && (trimmed[0] == '"' || trimmed[0] == '\''))
        if (trimmed.size() < 2 || trimmed.back() != trimmed.front()) throw std::runtime_error("Close the quotes around the model list");
}

// Replace only changed values; retain comments, whitespace and unknown settings.
inline std::string PatchIni(const std::string &source, const IniChanges &changes) {
    auto remaining = changes;
    std::string result, section;
    for (size_t pos = 0; pos < source.size();) {
        auto end = source.find('\n', pos);
        if (end == source.npos) end = source.size(); else ++end;
        auto line = source.substr(pos, end - pos);
        auto trimmed = Trim(line);
        if (trimmed.starts_with('[')) {
            auto close = trimmed.find(']');
            if (close != trimmed.npos) section = trimmed.substr(1, close - 1);
        } else if (!trimmed.empty() && trimmed[0] != '#' && trimmed[0] != ';') {
            auto eq = line.find('=');
            if (eq != line.npos) {
                auto key = Trim(std::string_view(line).substr(0, eq));
                auto match = remaining.find({section, key});
                if (match != remaining.end()) {
                    auto start = line.find_first_not_of(" \t", eq + 1);
                    if (start == line.npos) start = eq + 1;
                    bool quoted = false;
                    size_t finish = start;
                    for (; finish < line.size(); ++finish) {
                        if (line[finish] == '"') quoted = !quoted;
                        if (!quoted && (line[finish] == '#' || line[finish] == ';' || line[finish] == '\r' || line[finish] == '\n')) break;
                    }
                    while (finish > start && (line[finish - 1] == ' ' || line[finish - 1] == '\t')) --finish;
                    line.replace(start, finish - start, match->second);
                    // Replace duplicate keys too: the parser uses the final occurrence.
                }
            }
        }
        result += line;
        pos = end;
    }
    // Missing default settings are valid additions. Append an explicit section.
    for (const auto &[key, value] : remaining) {
        bool found = false;
        std::string active;
        for (size_t pos = 0; pos < source.size();) {
            auto end = source.find('\n', pos);
            if (end == source.npos) end = source.size();
            auto line = Trim(std::string_view(source).substr(pos, end - pos));
            if (line.starts_with('[') && line.find(']') != line.npos) active = line.substr(1, line.find(']') - 1);
            auto eq = line.find('=');
            if (active == key.first && eq != line.npos && Trim(std::string_view(line).substr(0, eq)) == key.second) found = true;
            pos = end + 1;
        }
        if (!found) result += "\r\n[" + key.first + "]\r\n" + key.second + " = " + value + "\r\n";
    }
    return result;
}

inline const Json &Templates() {
    // These are the keys consumed by this checkout, not the historic wiki schema.
    static const auto values = Json::parse(R"json({
      "metadata": {"author":"", "version":"", "creationtime":"", "desc":"", "minver":30100},
      "neon": {"mode":"static", "color":"#00AAFFFF", "smooth":true, "speed":1.0, "requires_lights":true, "size":{"x":1.0,"y":1.0}, "offset":{"x":0.0,"y":0.0}},
      "colors": {"white":{"red":255,"green":255,"blue":255,"alpha":255}},
      "leds": {"leds":{"material":{"color":"#FFFFFF","color_off":"#101010"}}},
      "sound": {"door_chime":true,"brake_pad":false},
      "plate": {"material":{"color":{"red":255,"green":255,"blue":255}, "color_off":{"red":128,"green":128,"blue":128}}},
      "lights": {"headlights":{"inertia":0.0,"material":{"color":"#FFFFFF","color_off":"#101010"},"corona":{"size":0.4,"type":"directional","color":{"red":255,"green":255,"blue":255,"alpha":80}},"shadow":{"size":1.0,"texture":"", "rotationchecks":true,"offset":{"x":0.0,"y":0.0},"color":{"red":255,"green":255,"blue":255,"alpha":80}}}},
      "exhausts": {"x_exhaust":{"lifetime":0.25,"speed":1.0,"size":0.85,"nitro_effect":true,"color":{"red":190,"green":190,"blue":190,"alpha":75}}},
      "roofs": {"x_convertible_roof":{"rotation":60.0,"speed":1.5}},
      "spoilers": {"movspoiler":{"rotation":30.0,"time":3000.0,"triggerspeed":20.0}},
      "doors": {"x_sd_lf":{"movmul":1.0,"popout":0.15}, "x_rd_lf":{"mul":1.0,"popout":0.15}},
      "gauges": {"x_rpm":{"maxrpm":8000,"maxrotation":260.0}, "x_sm":{"maxspeed":240,"maxrotation":260.0,"kph":true}, "x_tm":{"maxturbo":220.0,"maxrotation":220.0}, "x_odometer":{"kph":true}, "x_gasmeter":{"minangle":30.0,"maxangle":120.0}},
      "clocks": {"x_dclock":{"12hformat":false}},
      "rollback_bed": {"hydraulics":{"target_rot":0.0,"rot_speed":1.0,"target_move":2.0,"move_speed":1.0},"bed":{"rot_speed":1.0,"target_rot":0.0}},
      "carcols": {"colors":[{"red":255,"green":255,"blue":255}], "variations":[{"primary":0,"secondary":0,"tertiary":0,"quaternary":0}]},
      "sirens": {"states":{"1. default":{"1":{"size":0.6,"color":{"red":255,"green":0,"blue":0,"alpha":255},"pattern":[100,100],"type":"non-directional","inertia":0.0,"shadow":{"size":0.0,"type":"round","offset":0.0,"angleoffset":0.0}}}}}
    })json");
    return values;
}

inline bool Matches(const Json &value, const Json &example) {
    if (example.is_number()) return value.is_number() && std::isfinite(value.get<double>());
    return value.type() == example.type();
}

inline void CheckShape(const Json &value, const Json &example, const std::string &path) {
    const auto slash = path.rfind('/');
    const auto key = path.substr(slash == path.npos ? 0 : slash + 1);
    const bool flexibleColor = (key == "color" || key == "color_off")
        && !path.starts_with("exhausts/") && path.find("/corona/") == path.npos && path.find("/shadow/") == path.npos;
    if (flexibleColor && !example.is_boolean()) return; // The consumer accepts RGB objects, arrays, hex and named references.
    if (value.is_number() && ((path == "neon/size") || path.ends_with("/shadow/offset"))) return;
    if (value.is_array() && path.ends_with("/shadow/offset")) {
        if (value.empty() || value.size() > 2) throw std::runtime_error(path + ": expected one or two numbers");
        for (const auto &number : value) if (!number.is_number()) throw std::runtime_error(path + ": expected numbers");
        return;
    }
    if (!Matches(value, example)) throw std::runtime_error(path + ": wrong value type");
    if (example.is_object()) {
        for (const auto &[key, item] : value.items())
            if (example.contains(key)) CheckShape(item, example.at(key), path + "/" + key);
    }
}

inline void CheckColor(const Json &color, const Json &references, unsigned depth = 0) {
    if (depth > 32) throw std::runtime_error("Color reference cycle or nesting limit");
    if (color.is_string()) {
        const auto &name = color.get_ref<const std::string &>();
        if (references.is_object() && references.contains(name)) CheckColor(references.at(name), references, depth + 1);
        return; // Unknown names or invalid hex use the consumer's normal fallback.
    }
    if (color.is_array()) {
        if (color.size() < 3 || color.size() > 4) throw std::runtime_error("RGB color requires 3 or 4 channels");
        for (const auto &channel : color)
            if (!channel.is_number() || channel.get<double>() < 0 || channel.get<double>() > 255) throw std::runtime_error("Color channel must be numeric 0..255");
    } else if (color.is_object()) {
        for (const auto *key : {"red","green","blue","alpha","r","g","b","a"})
            if (color.contains(key) && (!color.at(key).is_number() || color.at(key).get<double>() < 0 || color.at(key).get<double>() > 255)) throw std::runtime_error("Color channel must be numeric 0..255");
    } else throw std::runtime_error("Color requires RGB, hex or a reference");
}

// Check every newly editable consumer boundary before touching live state.
inline void Validate(const Json &value, bool referenceOnly = false) {
    if (!value.is_object()) throw std::runtime_error("Config must be an object");
    const auto &templates = Templates();
    const auto noReferences = Json::object();
    const auto &colors = value.contains("colors") ? value.at("colors") : noReferences;
    if (!colors.is_object()) throw std::runtime_error("colors must be a named color map");
    for (const auto &color : colors) CheckColor(color, colors);
    for (const auto &[section, item] : value.items()) {
        if (section == "spotlights" || section == "spotlight")
            throw std::runtime_error(section + " is unsupported; configure lights/spotlights/material instead");
        if (section == "plate" && item.is_object() && (item.contains("color") || item.contains("color_off")))
            throw std::runtime_error("plate colors must be inside material");
        if (!templates.contains(section)) continue; // Preserve third-party metadata.
        if (section == "neon" && (item.is_string() || item.is_array())) { CheckColor(item, colors); continue; }
        if (!item.is_object()) throw std::runtime_error(section + ": expected object");
        if (section == "lights" || section == "exhausts" || section == "roofs" || section == "spoilers" || section == "doors" || section == "gauges" || section == "clocks" || section == "leds") {
            for (const auto &[name, settings] : item.items()) {
                if (section == "lights" && (name == "inertia" || name == "strobedelay" || name == "highbeam_pointlight_mul" || name == "highbeam_shadow_mul")) {
                    if (!settings.is_number()) throw std::runtime_error("lights/" + name + ": expected number");
                    continue;
                }
                if (section == "lights" && name == "plate") {
                    if (settings.is_object() && (settings.contains("color") || settings.contains("color_off")))
                        throw std::runtime_error("lights/" + name + " colors must be inside material");
                    CheckShape(settings, templates.at("plate"), section + "/" + name);
                    continue;
                }
                if ((section == "lights" || section == "leds") && settings.is_object()
                    && (settings.contains("color") || settings.contains("color_off")))
                    throw std::runtime_error(section + "/" + name + " colors must be inside material");
                auto prototype = templates.at(section).begin().value();
                if (section == "lights") prototype["strobedelay"] = 1000;
                if (section == "doors") prototype.update(templates.at("doors").at("x_rd_lf"));
                if (section == "gauges") for (const auto &gauge : templates.at(section)) prototype.update(gauge);
                CheckShape(settings, prototype, section + "/" + name);
            }
        } else if (section != "sirens") CheckShape(item, templates.at(section), section);
        if (section == "neon") {
            for (const auto *key : {"rainbow", "vehicle"}) if (item.contains(key) && !item.at(key).is_boolean()) throw std::runtime_error("neon mode flag must be boolean");
        }
    }
    // Reject unsafe values even in optional or custom nodes.
    const auto walk = [&](auto &&self, const Json &node, const std::string &path, unsigned depth) -> void {
        if (depth > 32) throw std::runtime_error("Config nesting limit: 32");
        if (node.is_object() || node.is_array()) for (const auto &[key, child] : node.items()) {
            const auto first = path.size() > 1 ? path.substr(1, path.find('/', 1) - 1) : std::string();
            if ((templates.contains(first) || first == "leds")
                && (key == "color" || key == "color_off") && !child.is_boolean()) CheckColor(child, colors);
            if (child.is_number()) {
                double number = child.get<double>();
                if (!std::isfinite(number) || std::abs(number) > 1000000.0) throw std::runtime_error(path + "/" + key + ": number out of range");
                if (key == "red" || key == "green" || key == "blue" || key == "alpha" || key == "r" || key == "g" || key == "b" || key == "a")
                    if (number < 0 || number > 255) throw std::runtime_error(path + "/" + key + ": color must be 0..255");
                if ((key == "maxrpm" || key == "maxspeed") && (number < 1 || std::floor(number) != number)) throw std::runtime_error(path + "/" + key + ": must be a positive integer");
                if (key == "strobedelay" && number < 0)
                    throw std::runtime_error(path + "/" + key + ": must be nonnegative");
                if (key == "maxrpm" || key == "maxspeed" || key == "maxturbo")
                    if (number <= 0) throw std::runtime_error(path + "/" + key + ": must be positive");
                if (key == "time" && number < 0)
                    throw std::runtime_error(path + "/" + key + ": must be nonnegative");
                if (key == "size" || key == "lifetime" || key == "inertia"
                    || ((first == "roofs" || first == "exhausts") && key == "speed")
                    || (first == "rollback_bed" && (key == "move_speed" || key == "rot_speed"))
                    || (path == "/neon/size" && (key == "x" || key == "y")))
                    if (number < 0) throw std::runtime_error(path + "/" + key + ": must be nonnegative");
            }
            self(self, child, path + "/" + key, depth + 1);
        }
    };
    walk(walk, value, "", 0);
    if (value.dump().size() > 1024 * 1024) throw std::runtime_error("Studio config limit: 1 MiB");
    if (value.contains("carcols")) {
        const auto &cols = value.at("carcols");
        if (!cols.contains("colors") || !cols.at("colors").is_array() || !cols.contains("variations") || !cols.at("variations").is_array())
            throw std::runtime_error("carcols requires colors and variations arrays");
        for (const auto &color : cols.at("colors")) {
            CheckShape(color, templates.at("carcols").at("colors")[0], "carcols/colors");
            for (const auto *key : {"red","green","blue"}) if (!color.contains(key) || !color.at(key).is_number()) throw std::runtime_error("carcols color requires RGB");
        }
        for (const auto &variation : cols.at("variations")) {
            CheckShape(variation, templates.at("carcols").at("variations")[0], "carcols/variations");
            for (const auto *key : {"primary","secondary","tertiary","quaternary"}) {
                int index = variation.value(key, 0);
                if (index < 0 || static_cast<size_t>(index) >= cols.at("colors").size()) throw std::runtime_error("carcols variation index outside color array");
            }
        }
    }
    if (value.contains("sirens")) {
        const auto &sirens = value.at("sirens");
        if (sirens.contains("imvehft") && !sirens.at("imvehft").is_boolean() && !sirens.at("imvehft").is_number()) throw std::runtime_error("sirens/imvehft must be a boolean or number");
        if (sirens.contains("references")) {
            const auto &refs = sirens.at("references");
            if (!refs.is_object()) throw std::runtime_error("siren references must be an object");
            for (const auto &[name, reference] : refs.items()) {
                if (!reference.is_object()) throw std::runtime_error("siren reference must be an object");
                if (name == "colors") for (const auto &color : reference) CheckColor(color, noReferences);
                else {
                    // Validate reference parameters using the same consumer boundaries.
                    Json check = {{"sirens", {{"states", {{"check", {{"1", reference}}}}}}}};
                    if (refs.contains("colors")) check["sirens"]["references"]["colors"] = refs.at("colors");
                    Validate(check, true);
                }
            }
        }
        const auto &states = sirens.contains("states") ? sirens.at("states") : sirens;
        if (states.size() > 64) throw std::runtime_error("siren state limit: 64");
        if (!states.is_object() && !states.is_array()) throw std::runtime_error("sirens states must be object or array");
        for (const auto &[stateName,state] : states.items()) {
            if (!sirens.contains("states") && (stateName=="references" || stateName=="imvehft")) continue;
            if (!state.is_object()) throw std::runtime_error("siren state must be object");
            for (const auto &[key, material] : state.items()) {
                if (key == "paintjob") { if (!material.is_number_integer()) throw std::runtime_error("paintjob must be integer"); continue; }
                if (key == "sound" || key == "audio") { if (!material.is_string()) throw std::runtime_error("siren sound must be string"); continue; }
                if (key.empty() || key[0] < '0' || key[0] > '9') continue;
                size_t consumed = 0;
                auto index = std::stoi(key, &consumed);
                if (consumed != key.size() || index < 0) throw std::runtime_error("siren material index must be a nonnegative integer");
                CheckShape(material, templates.at("sirens")["states"]["1. default"]["1"], "sirens/" + key);
                if (!referenceOnly && material.value("type", std::string()) == "rotator" && !material.contains("rotator") && !material.contains("reference")) throw std::runtime_error("rotator type requires a rotator object");
                if (material.contains("delay") && (!material.at("delay").is_number() || material.at("delay").get<double>() < 0)) throw std::runtime_error("siren delay must be nonnegative");
                if (material.contains("imvehft") && !material.at("imvehft").is_boolean() && !material.at("imvehft").is_number()) throw std::runtime_error("siren material imvehft must be a boolean or number");
                if (material.contains("rotator")) CheckShape(material.at("rotator"), Json{{"direction","clockwise"},{"type","linear"},{"time",1000},{"offset",0.0},{"radius",360.0}}, "sirens/rotator");
                if (material.contains("colors")) {
                    const auto &sequence = material.at("colors");
                    if (!sequence.is_array() || sequence.size() > 1024) throw std::runtime_error("siren color sequence must be an array of up to 1024 steps");
                    for (const auto &step : sequence) {
                        if (!step.is_array() || step.size() != 2 || !step[0].is_number_integer() || step[0].get<int>() < 0 || step[0].get<int>() > 1000000) throw std::runtime_error("siren color step requires time 0..1000000 ms and RGBA");
                        const auto *color = &step[1];
                        if (color->is_string() && sirens.contains("references") && sirens.at("references").contains("colors")) {
                            const auto &refs = sirens.at("references").at("colors");
                            const auto &name = color->get_ref<const std::string &>();
                            if (refs.contains(name)) color = &refs.at(name);
                        }
                        if (!color->is_object()) throw std::runtime_error("siren sequence color requires a complete RGBA object");
                        for (const auto *channel : {"red","green","blue","alpha"}) if (!color->contains(channel) || !color->at(channel).is_number()) throw std::runtime_error("siren sequence color requires all RGBA channels");
                        CheckColor(*color, noReferences);
                    }
                }
                if (material.contains("pattern")) {
                    size_t expanded = 0;
                    for (const auto &step : material.at("pattern")) {
                        if (step.is_array()) {
                            if (step.size() < 2 || !step[0].is_number_integer() || step[0].get<int>() < 1 || step[0].get<int>() > 1024) throw std::runtime_error("invalid repeated siren pattern");
                            expanded += step[0].get<int>() * (step.size() - 1);
                            for (size_t i = 1; i < step.size(); ++i) if (!step[i].is_number_integer() || step[i].get<int>() < 0 || step[i].get<int>() > 1000000) throw std::runtime_error("pattern time must be 0..1000000 ms");
                        } else {
                            if (!step.is_number_integer() || step.get<int>() < 0 || step.get<int>() > 1000000) throw std::runtime_error("pattern time must be 0..1000000 ms");
                            ++expanded;
                        }
                    }
                    if (expanded > 1024) throw std::runtime_error("siren pattern limit: 1024 steps");
                }
            }
        }
    }
}
}
