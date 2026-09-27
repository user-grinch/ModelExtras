#include "pch.h"
#include "defines.h"
#include "fileconverter.h"
#include <sstream>

#define DEFAULT_SIREN_SHADOW "round"

int Helper_ImVehFtReadColor(std::string input)
{
    if (input.length() == 3)
    {
        return std::stoi(input);
    }

    std::istringstream stream(input);
    int color = 0;
    stream >> std::hex >> color;
    return color;
}

bool Helper_MoveToBackup(const std::string &src)
{
    std::string backupDir = MOD_DATA_PATH("data\\backup\\");
    std::filesystem::create_directories(backupDir);

    std::filesystem::path source(src);
    std::filesystem::path destination = backupDir + source.filename().string();
    try
    {
        std::filesystem::rename(source, destination);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        LOG(ERROR) << std::format("Failed to move {} to backup: {}", src, e.what());
        return false;
    }

    return true;
}

#include <windows.h>

bool Helper_OpenFile(const std::string &path, std::ifstream &infile, const std::string &logPrefix)
{
    auto p = std::filesystem::path(path);
    p.make_preferred();
    infile.open(p);
    if (!infile)
    {
        LOG(WARNING) << std::format("{}: Failed to open {}", logPrefix, p.string());
        return false;
    }
    return true;
}

bool Helper_CreateFile(const std::string &path, std::ofstream &outfile, const std::string &logPrefix)
{
    auto p = std::filesystem::path(path);
    p.make_preferred();
    outfile.open(p);
    if (!outfile)
    {
        LOG(ERROR) << std::format("{}: Failed to create {} (Err: {})", logPrefix, p.string(), GetLastError());
        return false;
    }
    return true;
}

void Helper_LoadPrepJson(const std::string &path, nlohmann::json &jsonData, const std::string &logPrefix, const std::string &clearKey)
{
    auto p = std::filesystem::path(path);
    p.make_preferred();
    if (std::filesystem::exists(p))
    {
        std::ifstream temp(p);
        if (temp)
        {
            try
            {
                jsonData = nlohmann::json::parse(temp, nullptr, true, true);
            }
            catch (const std::exception &e)
            {
                LOG(WARNING) << std::format("{}: Failed to parse existing JSON {}: {}", logPrefix, path, e.what());
                jsonData = nlohmann::json::object();
            }
            temp.close();
        }

        LOG(WARNING) << std::format("{}: Merging with {}", logPrefix, path);
        if (jsonData.contains(clearKey))
        {
            LOG(WARNING) << std::format("{}: {} already contains {}, replacing...", logPrefix, path, clearKey);
            jsonData[clearKey] = {};
        }
    }
}

const char* GetShadowTypeName(int index) {
    static const char* shadowTypeNames[] = {
        "round", "pointlight", "arealight", "bollard", "comet",
        "cylindernarrow", "defined", "defineddiffuse", "defineddiffusespot", "definedspot",
        "narrow", "jellyfish", "mediumscatter", "overhead", "parallelbeam",
        "pear", "round", "scatterlight", "softarrow", "softdisplay",
        "star", "starfocused", "threelobeumbrella", "threelobevee", "tightfocused",
        "toppost", "trapezoid", "umbrella", "vee", "veeup",
        "xarrow", "xarrowdiffuse", "xarrowsoft"
    };

    constexpr int count = sizeof(shadowTypeNames) / sizeof(shadowTypeNames[0]);

    if (index < 0 || index >= count) {
        return DEFAULT_SIREN_SHADOW;
    }

    return shadowTypeNames[index];
}

void Helper_UpdateAVSRecursive(nlohmann::json& j) {
    if (j.is_object()) {
        for (auto& [key, value] : j.items()) {
            if (key == "shadow" && value.is_object()) {
                if (value.contains("type") && value["type"].is_number()) {
                    std::string name = GetShadowTypeName(value["type"].get<int>());
                    value["type"] = name;
                    if (name != DEFAULT_SIREN_SHADOW)
                    {
                        if (value.contains("size") && value["size"].is_number())
                        {
                            value["size"] = value["size"].get<float>() * 2.0f / 3.0f;
                        }
                    }
                }
            }
            Helper_UpdateAVSRecursive(value);
        }
    } else if (j.is_array()) {
        for (auto& item : j) {
            Helper_UpdateAVSRecursive(item);
        }
    }
}

bool Parse_EmlToMemory(std::istream &infile, nlohmann::json &jsonData, int &outModel)
{
    std::string line;
    int model = -1;

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        if (!(iss >> model))
        {
            return false;
        }
        break;
    }

    if (model <= 0)
        return false;

    outModel = model;

    jsonData["metadata"]["author"] = "Unknown";
    jsonData["metadata"]["desc"] = "Converted from ImVehFt";
    jsonData["metadata"]["minver"] = 20000;
    jsonData["sirens"]["imvehft"] = true;
    auto &extras = jsonData["sirens"]["states"]["1. modelextras"];

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        int id = 0, parent = 0, type = 0, switches = 0, starting = 0;
        int red = 255, green = 255, blue = 255, alpha = 255;
        float size = 0.5f, flash = 0.0f, shadow = 0.0f;
        std::string tempColor;

        if (!(iss >> id >> parent))
            continue;
        if (!(iss >> tempColor))
            continue;
        red = Helper_ImVehFtReadColor(tempColor);
        if (!(iss >> tempColor))
            continue;
        green = Helper_ImVehFtReadColor(tempColor);
        if (!(iss >> tempColor))
            continue;
        blue = Helper_ImVehFtReadColor(tempColor);
        if (!(iss >> tempColor))
            continue;
        alpha = Helper_ImVehFtReadColor(tempColor);
        if (!(iss >> type >> size >> shadow >> flash))
            continue;
        if (!(iss >> switches >> starting))
            continue;

        std::vector<uint64_t> pattern;
        uint64_t count = 0;
        for (int i = 0; i < switches; i++)
        {
            std::string t;
            if (!(iss >> t))
                continue;
            try
            {
                uint64_t val = std::stoull(t);
                uint64_t ms = (val >= count) ? (val - count) : 0;
                count = val;
                if (ms == 0)
                    continue;
                pattern.push_back(ms);
            }
            catch (...) {}
        }

        if (count == 0 || count > 64553)
        {
            starting = 1;
            pattern.clear();
        }

        auto &state = extras[std::to_string(id)];
        state["size"] = size;
        state["color"] = {{"red", red}, {"green", green}, {"blue", blue}, {"alpha", alpha}};
        state["state"] = starting;
        state["pattern"] = pattern;
        state["shadow"]["angleoffset"] = type == 1 ? 180.0f : 0.0f;
        state["shadow"]["size"] = shadow / 1.5f;
        state["inertia"] = flash / 100.0f;
        state["shadow"]["type"] = type == 2 ? "pointlight" : "round";
        state["type"] = type == 0 ? "directional" : (type == 1 ? "inversed-directional" : "non-directional");
    }

    return true;
}

bool Parse_AvsJsonToMemory(std::istream &infile, nlohmann::json &jsonData)
{
    jsonData["metadata"]["author"] = "Unknown";
    jsonData["metadata"]["desc"] = "Converted from AVS";
    jsonData["metadata"]["minver"] = 20000;
    try
    {
        jsonData["sirens"] = nlohmann::json::parse(infile);
        Helper_UpdateAVSRecursive(jsonData);
        return true;
    }
    catch (const std::exception &e)
    {
        LOG(ERROR) << std::format("Failed to parse AVS JSON: {}", e.what());
        return false;
    }
}

bool Parse_IvfcToMemory(std::istream &infile, nlohmann::json &jsonData, int &outModel)
{
    std::string line;
    int model = -1;
    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.rfind("vehicle_id", 0) == 0)
        {
            std::istringstream iss(line);
            std::string key;
            iss >> key >> model;
            break;
        }
    }

    if (model <= 0)
    {
        return false;
    }
    outModel = model;

    jsonData["metadata"]["author"] = "Unknown";
    jsonData["metadata"]["desc"] = "Converted from IVF";
    jsonData["metadata"]["minver"] = 20000;

    bool parsingColors = false, parsingVariations = false;
    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.starts_with("num_colors"))
            parsingColors = true, parsingVariations = false;
        else if (line.starts_with("num_variations"))
            parsingColors = false, parsingVariations = true;
        else
        {
            std::istringstream iss(line);
            if (parsingColors)
            {
                int r = 0, g = 0, b = 0;
                if (iss >> r >> g >> b)
                    jsonData["carcols"]["colors"].push_back({{"red", r}, {"green", g}, {"blue", b}});
            }
            else if (parsingVariations)
            {
                int a = 0, b = 0, c = 0, d = 0;
                if (iss >> a >> b >> c >> d)
                    jsonData["carcols"]["variations"].push_back({{"primary", a}, {"secondary", b}, {"tertiary", c}, {"quaternary", d}});
            }
        }
    }
    return true;
}

int Convert_EmlToJsonc(const std::string &emlPath)
{
    std::ifstream infile;
    if (!Helper_OpenFile(emlPath, infile, "EML2JSONC"))
        return -1;

    nlohmann::json jsonData;
    int model = -1;
    if (!Parse_EmlToMemory(infile, jsonData, model) || model <= 0)
    {
        LOG(WARNING) << std::format("EML2JSONC: Failed to parse model ID from {}", emlPath);
        infile.close();
        return -1;
    }
    infile.close();

    std::string jsonPath = MOD_DATA_PATH("data\\") + std::to_string(model) + ".jsonc";
    nlohmann::json baseJson;
    Helper_LoadPrepJson(jsonPath, baseJson, "EML2JSONC", "sirens");
    baseJson["metadata"] = jsonData["metadata"];
    baseJson["sirens"] = jsonData["sirens"];

    std::ofstream outfile;
    if (!Helper_CreateFile(jsonPath, outfile, "EML2JSONC"))
        return -1;

    outfile << baseJson.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(emlPath))
        return -1;
    LOG(INFO) << std::format("Successfully converted {} to {}", emlPath, jsonPath);
    return model;
}

void Convert_JsonToJsonc(const std::string &inPath)
{
    std::string outPath = inPath + "c";
    std::ifstream infile;
    if (!Helper_OpenFile(inPath, infile, "JSON2JSONC"))
        return;

    nlohmann::json parsedSirens;
    if (!Parse_AvsJsonToMemory(infile, parsedSirens))
    {
        infile.close();
        return;
    }
    infile.close();

    nlohmann::json baseJson;
    Helper_LoadPrepJson(outPath, baseJson, "JSON2JSONC", "sirens");
    baseJson["metadata"] = parsedSirens["metadata"];
    baseJson["sirens"] = parsedSirens["sirens"];

    std::ofstream outfile;
    if (!Helper_CreateFile(outPath, outfile, "JSON2JSONC"))
        return;

    outfile << baseJson.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(inPath))
        return;
    LOG(INFO) << std::format("Successfully converted {} to {}", inPath, outPath);
}

int Convert_IvfcToJsonc(const std::string &inPath)
{
    std::ifstream infile;
    if (!Helper_OpenFile(inPath, infile, "IVFC2JSONC"))
        return -1;

    nlohmann::json parsedCarcols;
    int model = -1;
    if (!Parse_IvfcToMemory(infile, parsedCarcols, model) || model <= 0)
    {
        LOG(WARNING) << std::format("IVFC2JSONC: Failed to parse model ID from {}", inPath);
        infile.close();
        return -1;
    }
    infile.close();

    std::string outPath = MOD_DATA_PATH("data\\") + std::to_string(model) + ".jsonc";
    nlohmann::json baseJson;
    Helper_LoadPrepJson(outPath, baseJson, "IVFC2JSONC", "carcols");
    baseJson["metadata"] = parsedCarcols["metadata"];
    baseJson["carcols"] = parsedCarcols["carcols"];

    std::ofstream outfile;
    if (!Helper_CreateFile(outPath, outfile, "IVFC2JSONC"))
        return -1;

    outfile << baseJson.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(inPath))
        return -1;
    LOG(INFO) << std::format("Successfully converted {} to {}", inPath, outPath);
    return model;
}
