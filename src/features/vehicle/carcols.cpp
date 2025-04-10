#include "pch.h"
#include "carcols.h"
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>

IVFCarcolsFeature IVFCarcols;
extern bool IsNightTime();

void IVFCarcolsFeature::Initialize()
{
    static CVehicle *pCurVehicle;
    injector::MakeInline<0x4C838D, 0x4C83AA>([](injector::reg_pack &regs)
                                             {
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
        } });

    injector::MakeInline<0x6D6617, 0x6D661C>([](injector::reg_pack &regs)
                                             {
        pCurVehicle = (CVehicle*)regs.esi;
        CVehicleModelInfo* pInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(pCurVehicle->m_nModelIndex);
        // CVehicle *__cdecl CVehicleModelInfo::SetupLightFlags(CVehicle *vehicle)
        plugin::Call<0x4C8C90>(pCurVehicle); });
}

void IVFCarcolsFeature::Parse(const nlohmann::json &data, int model)
{
    if (data.contains("carcols"))
    {
        auto &cols = data["carcols"]["colors"];
        auto &var = data["carcols"]["variations"];

        for (auto &e : var)
        {
            int pIdx = e.value("primary", 0);
            int sIdx = e.value("secondary", 0);
            int tIdx = e.value("tertiary", 0);
            int qIdx = e.value("quaternary", 0);

            auto &pCol = cols.at(pIdx);
            auto &sCol = cols.at(sIdx);
            auto &tCol = cols.at(tIdx);
            auto &qCol = cols.at(qIdx);

            CRGBA primaryColor = CRGBA(pCol["red"], pCol["green"], pCol["blue"], 255);
            CRGBA secondaryColor = CRGBA(sCol["red"], sCol["green"], sCol["blue"], 255);
            CRGBA tertiaryColor = CRGBA(tCol["red"], tCol["green"], tCol["blue"], 255);
            CRGBA quaternaryColor = CRGBA(qCol["red"], qCol["green"], qCol["blue"], 255);

            variations[model]
                .push_back({primaryColor, secondaryColor, tertiaryColor, quaternaryColor});
        }
    }
}