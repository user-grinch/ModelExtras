#include "pch.h"
#include "defines.h"
#include "utils/datamgr.h"
#include <string>
#include <CModelInfo.h>
#include "features/sirens.h"
#include "features/carcols.h"

bool is_number(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

void DataMgr::Init()
{
    LOG(INFO) << "Loading data files from ModelExtras/data...";
    for (const auto &e : std::filesystem::directory_iterator(MOD_DATA_PATH("data/")))
    {
        LoadFile(e);
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

        if (data.contains(model)) {
            LOG(WARNING) << std::format("Found multiple config files for model id {}", model);
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
                        LOG(WARNING) << text;
                    }
                }
                modelPath[model] = e.path().string();
            }
            else
            {
                LOG(ERROR) << std::format("Skipping {}. No metadata found!", filename);
                return;
            }

            data[model] = jsonData;

            Sirens::Parse(data[model], model);
            Carcols::Parse(data[model], model);
            LOG(INFO) << std::format("Registered file '{}'", filename);
        }
        catch (const nlohmann::json::parse_error &ex)
        {
            LOG(ERROR) << std::format("Failed to parse JSONC file '{}': {}", e.path().string(), ex.what());
        }
    }
    catch (const std::exception &ex)
    {
        std::u8string u8Path = e.path().u8string();
        std::string path(reinterpret_cast<const char *>(u8Path.data()), u8Path.size());
        LOG(ERROR) << std::format("Parsing {} failed. ({})", path, ex.what());
    }
}

nlohmann::json& DataMgr::Get(int model)
{
    return data[model];
}
