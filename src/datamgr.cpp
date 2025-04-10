#include "pch.h"
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
}

void DataMgr::Parse()
{
    std::string path = MOD_DATA_PATH("data/");

    for (const auto &e : std::filesystem::directory_iterator(path))
    {
        if (e.is_regular_file() && e.path().extension() == ".jsonc")
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
                data[model] = nlohmann::json::parse(file, NULL, true, true);
                if (gConfig.ReadBoolean("CONFIG", "ModelVersionCheck", true))
                {
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
                }

                Sirens::Parse(data[model], model);
                IVFCarcolsFeature::Parse(data[model], model);
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
