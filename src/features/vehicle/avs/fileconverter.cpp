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

int Convert_EmlToJsonc(const std::string &emlPath)
{
    std::ifstream infile(emlPath);

    if (!infile)
    {
        gLogger->warn("EML2JSONC: Failed to open {}", emlPath);
        return -1;
    }

    std::string line;
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
            gLogger->warn("EML2JSONC: Failed to parse model ID from {}", emlPath);
            return -1;
        }

        break;
    }

    std::string jsonPath = MOD_DATA_PATH("data\\") + std::to_string(model) + ".jsonc";
    nlohmann::json jsonData;

    if (std::filesystem::exists(jsonPath))
    {
        jsonData = nlohmann::json::parse(jsonPath, NULL, true, true);
        gLogger->warn("EML2JSONC: Merging {} with {}", emlPath, jsonPath);

        if (jsonData.contains("Sirens"))
        {
            gLogger->warn("EML2JSONC: {} file already contains siren data, replacing...", jsonPath);
            jsonData["Sirens"] = {};
        }
    }

    jsonData["Sirens"]["ImVehFt"] = true;
    jsonData["Sirens"]["states"] = {};
    jsonData["Sirens"]["states"]["1. ModelExtras"] = {};

    std::ofstream outfile(jsonPath);
    if (!outfile)
    {
        gLogger->error("Failed to create .jsonc file");
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

        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["size"] = size;
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["color"] = {{"red", red}, {"green", green}, {"blue", blue}, {"alpha", alpha}};
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["state"] = starting;
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["pattern"] = pattern;
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["shadow"]["size"] = shadow / 17.5f;
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["inertia"] = flash / 100.0f;
        jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["type"] = ((type == 0) ? "directional" : ((type == 1) ? "inversed-directional" : "non-directional"));

        // This is the only way ffs
        if (type == 0)
        {
            jsonData["Sirens"]["states"]["1. ModelExtras"][std::to_string(id)]["rot"] = 180.0f;
        }
    }

    infile.close();
    outfile << jsonData.dump(4);
    outfile.close();

    std::string backupDir = MOD_DATA_PATH("data\\backup\\");
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

void Convert_JsonToJsonc(const std::string &inPath)
{
    std::string outPath = inPath + "c";

    std::ifstream infile(inPath);
    if (!infile)
    {
        gLogger->error("JSON2JSONC: Failed to open {}", inPath);
        return;
    }

    std::ofstream outfile(outPath);
    if (!outfile)
    {
        gLogger->error("JSON2JSONC: Failed to create {}", outPath);
        return;
    }

    nlohmann::json inData, outData;
    inData = nlohmann::json::parse(infile);

    if (outData.contains("Sirens"))
    {
        gLogger->warn("JSON2JSONC: {} file already contains siren data, replacing...", outPath);
        outData["Sirens"] = {};
    }

    outData["Sirens"] = inData;
    outfile << outData.dump(4);
    infile.close();
    outfile.close();
    std::string backupDir = MOD_DATA_PATH("data\\backup\\");
    std::filesystem::create_directories(backupDir);

    std::filesystem::path source(inPath);
    std::filesystem::path destination = backupDir + source.filename().string();
    try
    {
        std::filesystem::rename(source, destination);
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        gLogger->error("Failed to move {} to backup: {}", inPath, e.what());
        return;
    }

    gLogger->info("Successfully converted {} to {}", inPath, outPath);
}