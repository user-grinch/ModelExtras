#include "pch.h"
#include "carcols.h"
#include <rwcore.h>
#include <rpworld.h>
#include <RenderWare.h>
#include "core/colors.h"

void IVFCarcols::Initialize()
{
    m_bEnabled = true;
}

CRGBA IVFCarcols::GetColor(CVehicle *pVeh, RpMaterial *pMat, CRGBA type) {
    CRGBA *colorTable = *reinterpret_cast<CRGBA**>(0x4C8390);

    int model = pVeh->m_nModelIndex;
    if (m_bEnabled && variations.contains(model)) {
        auto& data = ExData.Get(pVeh);
        int random = rand() % variations[model].size();
        auto col = variations[model][random];
        
        if (type == VEHCOL_PRIMARY) {
            if (!data.m_bPri) {
                data.m_Colors.primary = col.primary;
                data.m_bPri = true;
            }
            return data.m_Colors.primary;
        }
        else if (type == VEHCOL_SECONDARY) {
            if (!data.m_bSec) {
                data.m_Colors.secondary = col.secondary;
                data.m_bSec = true;
            }
            return data.m_Colors.secondary;
        }
        else if (type == VEHCOL_TERTIARY) {
            if (!data.m_bTer) {
                data.m_Colors.tert = col.tert;
                data.m_bTer = true;
            }
            return data.m_Colors.tert;
        }
        else if (type == VEHCOL_QUATARNARY) {
            if (!data.m_bQuat) {
                data.m_Colors.quart = col.quart;
                data.m_bQuat = true;
            }
            return data.m_Colors.quart;
        } else {
            return *reinterpret_cast<CRGBA*>(RpMaterialGetColor(pMat));
        }
    }
    else {
        int idx = 0;
        if (type == VEHCOL_PRIMARY) {
            idx = CVehicleModelInfo::ms_currentCol[0];
        }
        else if (type == VEHCOL_SECONDARY) {
            idx = CVehicleModelInfo::ms_currentCol[1];
        }
        else if (type == VEHCOL_TERTIARY) {
            idx = CVehicleModelInfo::ms_currentCol[2];
        }
        else if (type == VEHCOL_QUATARNARY) {
            idx = CVehicleModelInfo::ms_currentCol[3];
        }
        else {
            return *reinterpret_cast<CRGBA*>(RpMaterialGetColor(pMat));
        }
        return colorTable[idx];
    }
}

void IVFCarcols::Parse(const nlohmann::json &data, int model)
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