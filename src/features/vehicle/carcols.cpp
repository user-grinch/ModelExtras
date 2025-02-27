#include "pch.h"
#include "carcols.h"
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>

IVFCarcolsFeature IVFCarcols;
extern bool IsNightTime();

void IVFCarcolsFeature::Initialize() {
    parseFiles();

    static CVehicle* pCurVehicle;
    injector::MakeInline<0x4C838D, 0x4C83AA>([](injector::reg_pack& regs) {
        RpMaterial* pMat = (RpMaterial*)regs.eax;
        CRGBA color = *(CRGBA*)&pMat->color;
        color.a = 0;
        int model = pCurVehicle->m_nModelIndex;
        auto& data = ExData.Get(pCurVehicle);

        if (variations.contains(model)) {
            int random = rand() % variations[model].size();
            auto col = variations[model][random];
            CRGBA rgbaCol;
            if (color == CRGBA(60, 255, 0, 0)) {
                if (!data.m_bPri) {
                    data.m_Colors.primary = col.primary;
                    data.m_bPri = true;
                }
                rgbaCol = data.m_Colors.primary;
            }
            else if (color == CRGBA(255, 0, 175, 0)) {
                if (!data.m_bSec) {
                    data.m_Colors.secondary = col.secondary;
                    data.m_bSec = true;
                }
                rgbaCol = data.m_Colors.secondary;
            }
            else if (color == CRGBA(0, 255, 255, 0)) {
                if (!data.m_bTer) {
                    data.m_Colors.tert = col.tert;
                    data.m_bTer = true;
                }
                rgbaCol = data.m_Colors.tert;
            }
            else if (color == CRGBA(255, 0, 255, 0)) {
                if (!data.m_bQuat) {
                    data.m_Colors.quart = col.quart;
                    data.m_bQuat = true;
                }
                rgbaCol = data.m_Colors.quart;
            }
            else {
                return;
            }

            pMat->color.red = rgbaCol.r;
            pMat->color.green = rgbaCol.g;
            pMat->color.blue = rgbaCol.b;
        }
        else {
            int id = 0;
            if (color == CRGBA(60, 255, 0, 0)) {
                id = CVehicleModelInfo::ms_currentCol[0];
            }
            else if (color == CRGBA(255, 0, 175, 0)) {
                id = CVehicleModelInfo::ms_currentCol[1];
            }
            else if (color == CRGBA(0, 255, 255, 0)) {
                id = CVehicleModelInfo::ms_currentCol[2];
            }
            else if (color == CRGBA(255, 0, 255, 0)) {
                id = CVehicleModelInfo::ms_currentCol[3];
            }
            else {
                return;
            }

            pMat->color.red = CVehicleModelInfo::ms_vehicleColourTable[id].r;
            pMat->color.green = CVehicleModelInfo::ms_vehicleColourTable[id].g;
            pMat->color.blue = CVehicleModelInfo::ms_vehicleColourTable[id].b;
        }
    });

    injector::MakeInline<0x6D6617, 0x6D661C>([](injector::reg_pack& regs) {
        pCurVehicle = (CVehicle*)regs.esi;
        CVehicleModelInfo* pInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(pCurVehicle->m_nModelIndex);
        // CVehicle *__cdecl CVehicleModelInfo::SetupLightFlags(CVehicle *vehicle)
        plugin::Call<0x4C8C90>(pCurVehicle);
    });
}

void IVFCarcolsFeature::parseFiles() {
    std::string filePath = MOD_DATA_PATH("colors/");
    if (std::filesystem::exists(filePath) && std::filesystem::is_directory(filePath)) {
        for (const auto& entry : std::filesystem::directory_iterator(filePath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ivfc") {
                std::vector<CRGBA> colors;
                std::ifstream file(entry.path());
                if (!file) {
                    return;
                }

                std::string line;
                int currentVehicleId = -1;
                bool parsingColors = false, parsingVariations = false;

                while (std::getline(file, line)) {
                    size_t commentPos = line.find('#');
                    if (commentPos != std::string::npos) {
                        line = line.substr(0, commentPos);
                    }

                    line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
                    if (line.empty()) continue;

                    std::istringstream iss(line);

                    if (line.rfind("vehicle_id", 0) == 0) {
                        std::string key;
                        iss >> key >> currentVehicleId;
                    }
                    else if (line.starts_with("num_colors")) {
                        parsingColors = true;
                        parsingVariations = false;
                    }
                    else if (line.starts_with("num_variations")) {
                        parsingColors = false;
                        parsingVariations = true;
                    }
                    else if (currentVehicleId != -1) {
                        if (parsingColors) {
                            int r, g, b;
                            if (iss >> r >> g >> b) {
                                colors.emplace_back(r, g, b);
                            }
                        }
                        else if (parsingVariations) {
                            std::vector<int> variation;
                            int value;
                            while (iss >> value) {
                                variation.push_back(value);
                            }
                            if (!variation.empty() && variation.size() >= 3) {
                                CRGBA primary = colors[variation[0]];
                                CRGBA secondary = colors[variation[1]];
                                CRGBA tertiary = colors[variation[2]];
                                CRGBA quart = colors[variation[3]];

                                variations[currentVehicleId].push_back({
                                    primary, secondary, tertiary, quart
                                });
                            }
                        }
                    }
                }
            }
        }
    }
}