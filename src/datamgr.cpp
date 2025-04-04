#include "pch.h"
#include "datamgr.h"
#include <string>
#include <CModelInfo.h>

bool is_number(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

void DataMgr::Init()
{
    std::string path = MOD_DATA_PATH("data/");

    for (const auto &e : std::filesystem::directory_iterator(path))
    {
        if (e.is_regular_file() && e.path().extension() == ".json")
        {
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
                    continue; // invalid skip it
                }
            }

            if (model == 0)
            {
                continue; // skip it
            }

            std::ifstream file(e.path());
            try
            {
                data[model] = nlohmann::json::parse(file);
                if (data[model].contains("Metadata"))
                {
                    auto &info = data[model]["Metadata"];
                    int ver = info.value("MinVer", MOD_VERSION_NUMBER);
                    if (ver > MOD_VERSION_NUMBER)
                    {
                        std::string text = std::format("Model {} requires version [{}] but [{}] is installed.", model, ver, MOD_VERSION_NUMBER);
                        MessageBox(RsGlobal.ps->window, text.c_str(), "ModelExtras version mismatch!", MB_OK);
                        gLogger->warn(text);
                    }
                }
                LOG_VERBOSE("Successfully registered file '{}'", e.path().filename().string());
            }
            catch (const nlohmann::json::parse_error &ex)
            {
                gLogger->error("Failed to parse JSON in file '{}': {}", e.path().string(), ex.what());
            }
        }
    }
}

nlohmann::json &DataMgr::Get(int model)
{
    return data[model];
}
