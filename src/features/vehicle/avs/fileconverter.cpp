#include "pch.h"
#include <sstream>

int Helper_ImVehFtReadColor(std::string input)
{
    if (input.length() == 3)
    {
        return std::stoi(input);
    }

    std::istringstream stream(input);
    int color;
    stream >> std::hex >> color;
    return color;
};

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
        gLogger->error("Failed to move {} to backup: {}", src, e.what());
        return false;
    }

    return true;
}

bool Helper_OpenFile(const std::string &path, std::ifstream &infile, const std::string &logPrefix)
{
    infile.open(path);
    if (!infile)
    {
        gLogger->warn("{}: Failed to open {}", logPrefix, path);
        return false;
    }
    return true;
}

bool Helper_CreateFile(const std::string &path, std::ofstream &outfile, const std::string &logPrefix)
{
    outfile.open(path);
    if (!outfile)
    {
        gLogger->error("{}: Failed to create {}", logPrefix, path);
        return false;
    }
    return true;
}

void Helper_LoadPrepJson(const std::string &path, nlohmann::json &jsonData, const std::string &logPrefix, const std::string &clearKey)
{
    if (std::filesystem::exists(path))
    {
        std::ifstream temp(path);
        jsonData = nlohmann::json::parse(temp, NULL, true, true);
        temp.close();

        gLogger->warn("{}: Merging with {}", logPrefix, path);
        if (jsonData.contains(clearKey))
        {
            gLogger->warn("{}: {} already contains {}, replacing...", logPrefix, path, clearKey);
            jsonData[clearKey] = {};
        }
    }
}

int Convert_EmlToJsonc(const std::string &emlPath)
{
    std::ifstream infile;
    if (!Helper_OpenFile(emlPath, infile, "EML2JSONC"))
        return -1;

    std::string line;
    int model = -1;

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        if (!(iss >> model))
        {
            gLogger->warn("EML2JSONC: Failed to parse model ID from {}", emlPath);
            return -1;
        }
        break;
    }

    std::string jsonPath = MOD_DATA_PATH("data\\") + std::to_string(model) + ".jsonc";
    nlohmann::json jsonData;
    Helper_LoadPrepJson(jsonPath, jsonData, "EML2JSONC", "Sirens");

    jsonData["sirens"]["imvehft"] = true;
    auto &extras = jsonData["sirens"]["states"]["1. modelextras"];

    std::ofstream outfile;
    if (!Helper_CreateFile(jsonPath, outfile, "EML2JSONC"))
        return -1;

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        int id, parent, type, switches, starting;
        int red, green, blue, alpha;
        float size, flash, shadow;
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
            uint64_t ms = std::stoi(t) - count;
            count += ms;
            if (ms == 0)
                continue;
            pattern.push_back(ms);
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
        state["shadow"]["size"] = shadow / 2;
        state["inertia"] = flash / 100.0f;
        state["type"] = type == 0 ? "directional" : (type == 1 ? "inversed-directional" : "non-directional");
    }

    infile.close();
    outfile << jsonData.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(emlPath))
        return -1;
    gLogger->info("Successfully converted {} to {}", emlPath, jsonPath);
    return model;
}

void Convert_JsonToJsonc(const std::string &inPath)
{
    std::string outPath = inPath + "c";
    std::ifstream infile;
    if (!Helper_OpenFile(inPath, infile, "JSON2JSONC"))
        return;

    nlohmann::json jsonData;
    Helper_LoadPrepJson(outPath, jsonData, "JSON2JSONC", "Sirens");

    std::ofstream outfile;
    if (!Helper_CreateFile(outPath, outfile, "JSON2JSONC"))
        return;

    jsonData["sirens"] = nlohmann::json::parse(infile);
    infile.close();
    outfile << jsonData.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(inPath))
        return;
    gLogger->info("Successfully converted {} to {}", inPath, outPath);
}

int Convert_IvfcToJsonc(const std::string &inPath)
{
    std::ifstream infile;
    if (!Helper_OpenFile(inPath, infile, "IVFC2JSONC"))
        return -1;

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

    std::string outPath = MOD_DATA_PATH("data\\") + std::to_string(model) + ".jsonc";
    nlohmann::json jsonData;
    Helper_LoadPrepJson(outPath, jsonData, "IVFC2JSONC", "Carcols");

    std::ofstream outfile;
    if (!Helper_CreateFile(outPath, outfile, "IVFC2JSONC"))
        return -1;

    bool parsingColors = false, parsingVariations = false;
    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        if (line.starts_with("num_colors"))
            parsingColors = true, parsingVariations = false;
        else if (line.starts_with("num_variations"))
            parsingColors = false, parsingVariations = true;
        else if (model != -1)
        {
            std::istringstream iss(line);
            if (parsingColors)
            {
                int r, g, b;
                if (iss >> r >> g >> b)
                    jsonData["carcols"]["colors"].push_back({{"red", r}, {"green", g}, {"blue", b}});
            }
            else if (parsingVariations)
            {
                int a, b, c, d;
                if (iss >> a >> b >> c >> d)
                    jsonData["carcols"]["variations"].push_back({{"primary", a}, {"secondary", b}, {"tertiary", c}, {"quaternary", d}});
            }
        }
    }

    infile.close();
    outfile << jsonData.dump(4);
    outfile.close();

    if (!Helper_MoveToBackup(inPath))
        return -1;
    gLogger->info("Successfully converted {} to {}", inPath, outPath);
    return model;
}
