#include "pch.h"
#include <sstream>

int ImVehFt_ReadColor(std::string input)
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

int ImVehFt_EmlToJson(const std::string &emlPath)
{
    std::ifstream infile(emlPath);

    if (!infile)
    {
        gLogger->error("Failed to open .eml file");
        return -1;
    }

    std::string line;
    nlohmann::json jsonData;
    int model = -1;

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        if (!(iss >> model))
        {
            gLogger->error("Failed to parse model ID from .eml file");
            return -1;
        }
        jsonData["ImVehFt"] = true;
        jsonData["states"] = {};
        jsonData["states"]["1. ModelExtras"] = {};
        break;
    }

    std::string jsonPath = MOD_DATA_PATH("data\\sirens\\") + std::to_string(model) + ".json";
    std::ofstream outfile(jsonPath);
    if (!outfile)
    {
        gLogger->error("Failed to create .json file");
        return -1;
    }

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        int id, parent, type, switches, starting;
        int red, green, blue, alpha;
        float size, flash, shadow;
        std::string tempColor;

        if (!(iss >> id >> parent))
            continue;

        if (!(iss >> tempColor))
            continue;
        red = ImVehFt_ReadColor(tempColor);

        if (!(iss >> tempColor))
            continue;
        green = ImVehFt_ReadColor(tempColor);

        if (!(iss >> tempColor))
            continue;
        blue = ImVehFt_ReadColor(tempColor);

        if (!(iss >> tempColor))
            continue;
        alpha = ImVehFt_ReadColor(tempColor);

        if (!(iss >> type >> size >> shadow >> flash))
            continue;
        if (!(iss >> switches >> starting))
            continue;

        std::vector<uint64_t> pattern;
        uint64_t count = 0;

        for (int index = 0; index < switches; index++)
        {
            std::string string;
            if (!(iss >> string))
                continue;
            uint64_t milliseconds = std::stoi(string) - count;
            count += milliseconds;

            if (milliseconds == 0)
                continue;
            pattern.push_back(milliseconds);
        }

        if (count == 0 || count > 64553)
        {
            starting = 1;
            pattern.clear();
        }

        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["size"] = size;
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["color"] = {{"red", red}, {"green", green}, {"blue", blue}, {"alpha", alpha}};
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["state"] = starting;
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["pattern"] = pattern;
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["shadow"]["size"] = shadow / 17.5f;
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["inertia"] = flash / 100.0f;
        jsonData["states"]["1. ModelExtras"][std::to_string(id)]["type"] = ((type == 0) ? "directional" : ((type == 1) ? "inversed-directional" : "non-directional"));

        // This is the only way ffs
        if (type == 0)
        {
            jsonData["states"]["1. ModelExtras"][std::to_string(id)]["rot"] = 180.0f;
        }
    }

    infile.close();
    outfile << jsonData.dump(4);
    outfile.close();

    std::string backupDir = MOD_DATA_PATH("data\\sirens\\backup\\");
    std::filesystem::create_directories(backupDir);

    std::filesystem::path source(emlPath);
    std::filesystem::path destination = backupDir + source.filename().string();
    try
    {
        std::filesystem::rename(source, destination);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        gLogger->error("Failed to move {} to backup: {}", emlPath, e.what());
        return -1;
    }

    gLogger->info("Successfully converted {} to {}", emlPath, jsonPath);
    return model;
}