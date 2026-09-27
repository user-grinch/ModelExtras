#include "pch.h"
#include "defines.h"
#include "utils/datamgr.h"
#include "features/core/fileconverter.h"
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <game_sa/CModelInfo.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static bool is_number(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

static std::string ReadFileContent(const std::filesystem::path &p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open()) return {};
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    // Strip UTF-8 BOM if present
    if (str.size() >= 3 &&
        static_cast<unsigned char>(str[0]) == 0xEF &&
        static_cast<unsigned char>(str[1]) == 0xBB &&
        static_cast<unsigned char>(str[2]) == 0xBF)
    {
        str.erase(0, 3);
    }
    return str;
}

static std::filesystem::path GetSelfDirectory()
{
    std::error_code ec;
    char modulePath[MAX_PATH] = {0};
    if (GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), modulePath, MAX_PATH))
    {
        return std::filesystem::weakly_canonical(std::filesystem::path(modulePath).parent_path(), ec);
    }
    return {};
}

static std::unordered_map<std::string, int> ParseModLoaderPriorities(const std::filesystem::path &modloaderIniPath, std::vector<std::string> &outExcludedMods)
{
    std::unordered_map<std::string, int> priorities;
    std::error_code ec;
    if (!std::filesystem::exists(modloaderIniPath, ec))
        return priorities;

    std::ifstream file(modloaderIniPath);
    if (!file.is_open())
        return priorities;

    std::string line;
    std::string currentSection;
    auto trim = [](std::string &s) {
        size_t f = s.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) { s.clear(); return; }
        size_t l = s.find_last_not_of(" \t\r\n");
        s = s.substr(f, l - f + 1);
    };

    while (std::getline(file, line))
    {
        trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            std::transform(currentSection.begin(), currentSection.end(), currentSection.begin(), ::tolower);
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = line.substr(0, eqPos);
            std::string valStr = line.substr(eqPos + 1);

            trim(key);
            trim(valStr);

            size_t commentPos = valStr.find_first_of(";#");
            if (commentPos != std::string::npos)
            {
                valStr = valStr.substr(0, commentPos);
                trim(valStr);
            }

            if (key.empty() || valStr.empty()) continue;

            std::string keyLower = key;
            std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);

            if (keyLower == "excludemods")
            {
                std::istringstream ss(valStr);
                std::string item;
                while (std::getline(ss, item, ','))
                {
                    trim(item);
                    if (!item.empty())
                    {
                        std::transform(item.begin(), item.end(), item.begin(), ::tolower);
                        outExcludedMods.push_back(item);
                    }
                }
            }
            else if (currentSection == "priority" || currentSection.ends_with(".priority"))
            {
                try
                {
                    int p = std::stoi(valStr);
                    priorities[keyLower] = p;
                }
                catch (...) {}
            }
        }
    }
    return priorities;
}

void DataMgr::Init()
{
    gbModLoaderData = gConfig.ReadBoolean("CONFIG", "ModLoaderData", true);
    data.clear();
    modelPath.clear();

    // 1. Convert legacy files inside ModelExtras/data to .jsonc
    Convert();

    // 2. Load Base Layer from ModelExtras/data/
    LoadBaseData();

    // 3. Scan ModLoader directory (if present) and apply configs based on pure ModLoader priority
    LoadModLoaderData();

    // 4. Notify registered feature listeners
    for (const auto &[name, listener] : listeners)
    {
        if (listener)
        {
            for (const auto &[model, jsonData] : data)
            {
                listener(model, jsonData);
            }
        }
    }
}

void DataMgr::Convert()
{
    std::string path = std::string(MOD_DATA_PATH("data\\"));
    std::error_code ec;

    if (std::filesystem::exists(path, ec))
    {
        for (auto &p : std::filesystem::directory_iterator(path, ec))
        {
            std::string filePath = p.path().string();
            std::string fileExt = p.path().extension().string();
            if (fileExt == ".eml")
            {
                Convert_EmlToJsonc(filePath);
            }
            else if (fileExt == ".json")
            {
                Convert_JsonToJsonc(filePath);
            }
            else if (fileExt == ".ivfc")
            {
                Convert_IvfcToJsonc(filePath);
            }
        }
    }
}

void DataMgr::LoadBaseData()
{
    std::string dataPath = std::string(MOD_DATA_PATH("data/"));
    std::error_code ec;
    if (!std::filesystem::exists(dataPath, ec) || !std::filesystem::is_directory(dataPath, ec))
    {
        LOG(WARNING) << "ModelExtras/data directory doesn't exist or is not a directory";
        return;
    }

    LOG(INFO) << "Loading base data files from ModelExtras/data...";
    try
    {
        for (const auto &e : std::filesystem::directory_iterator(dataPath, ec))
        {
            LoadFile(e);
        }
    }
    catch (const std::exception &ex)
    {
        LOG(ERROR) << std::format("Failed to iterate data directory: {}", ex.what());
    }
}

void DataMgr::LoadModLoaderData()
{
    if (!gbModLoaderData)
    {
        return;
    }

    std::filesystem::path modloaderPath = std::filesystem::path("modloader");
    std::error_code ec;
    if (!std::filesystem::exists(modloaderPath, ec) || !std::filesystem::is_directory(modloaderPath, ec))
    {
        return;
    }

    // Determine ModelExtras's own installation folder to prevent self-scanning in modloader
    std::filesystem::path selfDir = GetSelfDirectory();

    std::vector<std::string> excludedMods;
    std::unordered_map<std::string, int> modPriorities = ParseModLoaderPriorities(modloaderPath / "modloader.ini", excludedMods);

    enum class ConfigFormat {
        Eml = 1,
        Ivfc = 2,
        AvsJson = 3,
        Jsonc = 4
    };

    struct ModCandidate {
        int model = 0;
        int modPriority = 50; // Pure ModLoader priority
        std::filesystem::path path;
        ConfigFormat format = ConfigFormat::Jsonc;
        std::string rootModName;
        bool isNumericName = false;
    };

    std::unordered_map<int, std::vector<ModCandidate>> candidatesByModel;

    try
    {
        auto iter = std::filesystem::recursive_directory_iterator(
            modloaderPath,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        auto endIter = std::filesystem::recursive_directory_iterator();

        while (iter != endIter)
        {
            const auto &entry = *iter;

            if (entry.is_directory(ec))
            {
                std::string folderName = entry.path().filename().string();
                // 1. Skip hidden / dot folders (.Backup, .data, .profiles, etc.)
                if (!folderName.empty() && folderName[0] == '.')
                {
                    iter.disable_recursion_pending();
                    iter.increment(ec);
                    continue;
                }

                // 2. Skip ModelExtras's own directory if installed inside modloader
                if (!selfDir.empty())
                {
                    std::error_code eqEc;
                    if (std::filesystem::equivalent(entry.path(), selfDir, eqEc))
                    {
                        iter.disable_recursion_pending();
                        iter.increment(ec);
                        continue;
                    }
                }

                // 3. Skip ignored mod folders (.ignore / modloader.ignore)
                if (std::filesystem::exists(entry.path() / ".ignore", ec) ||
                    std::filesystem::exists(entry.path() / "modloader.ignore", ec))
                {
                    iter.disable_recursion_pending();
                    iter.increment(ec);
                    continue;
                }

                // 4. Check if root mod directory is disabled in modloader.ini
                auto rel = std::filesystem::relative(entry.path(), modloaderPath, ec);
                if (!rel.empty())
                {
                    std::string rootMod = rel.begin()->string();
                    std::string rootModLower = rootMod;
                    std::transform(rootModLower.begin(), rootModLower.end(), rootModLower.begin(), ::tolower);

                    if (std::find(excludedMods.begin(), excludedMods.end(), rootModLower) != excludedMods.end())
                    {
                        iter.disable_recursion_pending();
                        iter.increment(ec);
                        continue;
                    }

                    auto pIt = modPriorities.find(rootModLower);
                    if (pIt != modPriorities.end() && pIt->second <= 0)
                    {
                        iter.disable_recursion_pending();
                        iter.increment(ec);
                        continue;
                    }
                }

                iter.increment(ec);
                continue;
            }

            if (!entry.is_regular_file(ec))
            {
                iter.increment(ec);
                continue;
            }

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            ConfigFormat fmt;
            if (ext == ".jsonc") fmt = ConfigFormat::Jsonc;
            else if (ext == ".json") fmt = ConfigFormat::AvsJson;
            else if (ext == ".eml") fmt = ConfigFormat::Eml;
            else if (ext == ".ivfc") fmt = ConfigFormat::Ivfc;
            else
            {
                iter.increment(ec);
                continue;
            }

            int modPriority = 50; // Default ModLoader priority
            std::string rootMod;
            auto rel = std::filesystem::relative(entry.path(), modloaderPath, ec);
            if (!rel.empty())
            {
                rootMod = rel.begin()->string();
                std::string rootModLower = rootMod;
                std::transform(rootModLower.begin(), rootModLower.end(), rootModLower.begin(), ::tolower);

                if (std::find(excludedMods.begin(), excludedMods.end(), rootModLower) != excludedMods.end())
                {
                    iter.increment(ec);
                    continue;
                }

                auto pIt = modPriorities.find(rootModLower);
                if (pIt != modPriorities.end())
                {
                    modPriority = pIt->second;
                    if (modPriority <= 0)
                    {
                        iter.increment(ec);
                        continue;
                    }
                }
            }

            int model = 0;
            bool isNumeric = false;
            if (fmt == ConfigFormat::Jsonc || fmt == ConfigFormat::AvsJson)
            {
                std::string stem = entry.path().stem().string();
                isNumeric = is_number(stem);
                if (isNumeric)
                {
                    model = std::stoi(stem);
                }
                else
                {
                    if (!CModelInfo::GetModelInfo(stem.c_str(), &model))
                    {
                        iter.increment(ec);
                        continue;
                    }
                }
            }
            else if (fmt == ConfigFormat::Eml)
            {
                isNumeric = true;
                std::string content = ReadFileContent(entry.path());
                std::istringstream peekFile(content);
                std::string line;
                while (std::getline(peekFile, line))
                {
                    if (line.empty() || line[0] == '#') continue;
                    std::istringstream iss(line);
                    iss >> model;
                    break;
                }
            }
            else if (fmt == ConfigFormat::Ivfc)
            {
                isNumeric = true;
                std::string content = ReadFileContent(entry.path());
                std::istringstream peekFile(content);
                std::string line;
                while (std::getline(peekFile, line))
                {
                    if (line.empty() || line[0] == '#') continue;
                    if (line.rfind("vehicle_id", 0) == 0)
                    {
                        std::istringstream iss(line);
                        std::string key;
                        iss >> key >> model;
                        break;
                    }
                }
            }

            if (model > 0 && model < 20000)
            {
                ModCandidate cand;
                cand.model = model;
                cand.modPriority = modPriority;
                cand.path = entry.path();
                cand.format = fmt;
                cand.rootModName = rootMod;
                cand.isNumericName = isNumeric;
                candidatesByModel[model].push_back(std::move(cand));
            }

            iter.increment(ec);
        }
    }
    catch (const std::exception &ex)
    {
        LOG(ERROR) << std::format("Failed during modloader directory scan: {}", ex.what());
    }

    LOG(INFO) << std::format("Applying configs from ModLoader (found {} models)...", candidatesByModel.size());

    for (auto &[model, candidates] : candidatesByModel)
    {
        // Sort candidates:
        // 1. ModLoader priority ascending (higher priority mod applies last and wins)
        // 2. If same priority: Named config (e.g. admiral.jsonc) wins over Numeric (e.g. 445.jsonc)
        // 3. Deterministic path comparison as ultimate tie-breaker
        std::sort(candidates.begin(), candidates.end(), [](const ModCandidate &a, const ModCandidate &b) {
            if (a.modPriority != b.modPriority)
                return a.modPriority < b.modPriority;
            if (a.isNumericName != b.isNumericName)
                return a.isNumericName > b.isNumericName; // false (named) goes after true (numeric) -> named wins
            return a.path.string() < b.path.string();
        });

        for (const auto &cand : candidates)
        {
            std::string content = ReadFileContent(cand.path);
            if (content.empty()) continue;

            try
            {
                if (cand.format == ConfigFormat::Jsonc)
                {
                    auto jsonData = nlohmann::json::parse(content, nullptr, true, true);
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
                    }
                    data[model] = std::move(jsonData);
                    modelPath[model] = cand.path.string();
                    LOG(INFO) << std::format("Loaded modloader JSONC (priority {}) for model {} from '{}'", cand.modPriority, model, cand.path.string());
                }
                else if (cand.format == ConfigFormat::AvsJson)
                {
                    std::istringstream stream(content);
                    nlohmann::json converted;
                    if (Parse_AvsJsonToMemory(stream, converted))
                    {
                        if (data.contains(model))
                        {
                            data[model]["sirens"] = std::move(converted["sirens"]);
                            if (!data[model].contains("metadata"))
                                data[model]["metadata"] = std::move(converted["metadata"]);
                        }
                        else
                        {
                            data[model] = std::move(converted);
                        }
                        modelPath[model] = cand.path.string();
                        LOG(INFO) << std::format("Loaded modloader AVS JSON (priority {}) for model {} from '{}'", cand.modPriority, model, cand.path.string());
                    }
                }
                else if (cand.format == ConfigFormat::Eml)
                {
                    std::istringstream stream(content);
                    nlohmann::json converted;
                    int emlModel = 0;
                    if (Parse_EmlToMemory(stream, converted, emlModel) && emlModel == model)
                    {
                        if (data.contains(model))
                        {
                            data[model]["sirens"] = std::move(converted["sirens"]);
                            if (!data[model].contains("metadata"))
                                data[model]["metadata"] = std::move(converted["metadata"]);
                        }
                        else
                        {
                            data[model] = std::move(converted);
                        }
                        modelPath[model] = cand.path.string();
                        LOG(INFO) << std::format("Loaded modloader EML (priority {}) for model {} from '{}'", cand.modPriority, model, cand.path.string());
                    }
                }
                else if (cand.format == ConfigFormat::Ivfc)
                {
                    std::istringstream stream(content);
                    nlohmann::json converted;
                    int ivfcModel = 0;
                    if (Parse_IvfcToMemory(stream, converted, ivfcModel) && ivfcModel == model)
                    {
                        if (data.contains(model))
                        {
                            data[model]["carcols"] = std::move(converted["carcols"]);
                            if (!data[model].contains("metadata"))
                                data[model]["metadata"] = std::move(converted["metadata"]);
                        }
                        else
                        {
                            data[model] = std::move(converted);
                        }
                        modelPath[model] = cand.path.string();
                        LOG(INFO) << std::format("Loaded modloader IVFC (priority {}) for model {} from '{}'", cand.modPriority, model, cand.path.string());
                    }
                }
            }
            catch (const std::exception &ex)
            {
                LOG(ERROR) << std::format("Failed to parse modloader file '{}': {}", cand.path.string(), ex.what());
            }
        }
    }
}

void DataMgr::Reload(int model)
{
    Init();
}

const std::string &DataMgr::GetPath(int model)
{
    return modelPath[model];
}

void DataMgr::LoadFile(const std::filesystem::directory_entry &e)
{
    try
    {
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
            if (!CModelInfo::GetModelInfo(key.c_str(), &model))
            {
                return;
            }
        }

        if (model <= 0 || model >= 20000)
        {
            return;
        }

        std::string content = ReadFileContent(e.path());
        if (content.empty())
        {
            return;
        }

        try
        {
            auto jsonData = nlohmann::json::parse(content, nullptr, true, true);

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
            }

            modelPath[model] = e.path().string();
            data[model] = std::move(jsonData);
            LOG(INFO) << std::format("Registered base file '{}' for model {}", filename, model);
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

bool DataMgr::Has(int model)
{
    return data.find(model) != data.end();
}

const nlohmann::json* DataMgr::Find(int model)
{
    auto it = data.find(model);
    return (it != data.end()) ? &it->second : nullptr;
}

nlohmann::json& DataMgr::Get(int model)
{
    auto it = data.find(model);
    if (it == data.end())
    {
        static nlohmann::json s_Empty = nlohmann::json::object();
        return s_Empty;
    }
    return it->second;
}

void DataMgr::RegisterListener(std::string_view name, ModelDataListener_t listener)
{
    listeners.push_back({std::string(name), listener});
    for (const auto &[model, jsonData] : data)
    {
        listener(model, jsonData);
    }
}
