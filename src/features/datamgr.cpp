#include "pch.h"
#include "defines.h"
#include "datamgr.h"
#include <string>
#include <CModelInfo.h>
#include "features/vehicle/sirens.h"
#include "features/vehicle/carcols.h"

bool is_number(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

extern int Convert_EmlToJsonc(const std::string &inPath);
extern void Convert_JsonToJsonc(const std::string &inPath);
extern int Convert_IvfcToJsonc(const std::string &inPath);

void DataMgr::Init()
{
    Convert();
    Parse();
}

void DataMgr::Convert()
{
    std::string path = std::string(MOD_DATA_PATH("data/"));

    if (std::filesystem::exists(path)) {
        for (auto &p : std::filesystem::directory_iterator(path))
        {
            std::string filePath = p.path().string();
            if (filePath.ends_with(".eml"))
            {
                Convert_EmlToJsonc(filePath);
            }
            else if (filePath.ends_with(".json"))
            {
                Convert_JsonToJsonc(filePath);
            }
            else if (filePath.ends_with(".ivfc"))
            {
                Convert_IvfcToJsonc(filePath);
            }
        }
    } else {
        gLogger->warn("ModelExtras/data directory doesn't exist");
    }
}

void DataMgr::Reload(int model)
{
    std::filesystem::directory_entry e = std::filesystem::directory_entry(std::filesystem::path(DataMgr::GetPath(model)));
    DataMgr::LoadFile(e);
}

const std::string &DataMgr::GetPath(int model)
{
    return modelPath[model];
}

void DataMgr::LoadFile(const std::filesystem::directory_entry &e)
{
    try
    {
        // Skipping directories or links here
        if (!e.is_regular_file() || e.is_directory() || e.path().extension() != ".jsonc")
        {
            return;
        }

        // Ignore folders with '.' int their names .data / .profile etc
        std::string parentPath = e.path().parent_path().string();
        if (STR_FOUND(parentPath, '.'))
        {
            return;
        }

        std::string filename = e.path().filename().string();
        std::string key = e.path().stem().string();
        int model = 0;

        if (is_number(key))
        {
            model = std::stoi(key);
        }
        else
        {
            if (!CModelInfo::GetModelInfo((char *)key.c_str(), &model))
            {
                return;
            }
        }

        if (model == 0)
        {
            return;
        }

        std::ifstream file(e.path());
        try
        {
            auto jsonData = nlohmann::json::parse(file, nullptr, true, true);

            if (jsonData.contains("metadata") && jsonData["metadata"].contains("minver"))
            {
                if (gConfig.ReadBoolean("CONFIG", "ModelVersionCheck", true))
                {
                    auto &info = jsonData["metadata"];
                    int ver = info.value("minver", MOD_VERSION_NUMBER);
                    if (ver > MOD_VERSION_NUMBER)
                    {
                        std::string text = std::format("Model {} requires version [{}] but [{}] is installed.", model, ver, MOD_VERSION_NUMBER);
                        MessageBox(RsGlobal.ps->window, text.c_str(), "ModelExtras version mismatch!", MB_OK);
                        gLogger->warn(text);
                    }
                }
                modelPath[model] = e.path().string();
            }
            else
            {
                gLogger->error("Skipping {}. No metadata found!", filename);
            }

            data[model] = jsonData;

            Sirens::Parse(data[model], model);
            IVFCarcols::Parse(data[model], model);
            gLogger->info("Successfully registered file '{}'", filename);
        }
        catch (const nlohmann::json::parse_error &ex)
        {
            gLogger->error("Failed to parse JSONC in file '{}': {}", e.path().string(), ex.what());
        }
    }
    catch (const std::exception &ex)
    {
        std::u8string u8Path = e.path().u8string();
        std::string path(reinterpret_cast<const char *>(u8Path.data()), u8Path.size());
        gLogger->error("Parsing {} failed. ({})", path, ex.what());
    }
}

void DataMgr::Parse()
{
    auto LoadModelDataFromPath = [&](const std::string &path)
    {
        for (const auto &e : std::filesystem::directory_iterator(path))
        {
            LoadFile(e);
        }
    };

    gLogger->info("Loading data files from ModelExtras/data...");
    LoadModelDataFromPath(MOD_DATA_PATH("data/"));
    LOG_NO_LEVEL("");
    if (GetModuleHandle("modloader.asi"))
    {
        gLogger->info("Loading data files from modloader...");
        LoadModelDataFromPath(GAME_PATH((char *)"modloader/"));
    }
}

nlohmann::json &
DataMgr::Get(int model)
{
    return data[model];
}
