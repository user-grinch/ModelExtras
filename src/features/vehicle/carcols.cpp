#include "carcols.h"
#include "core/colors.h"
#include "pch.h"
#include <RenderWare.h>
#include <rpworld.h>
#include <rwcore.h>

#define IS_SAME_COLOR(type, VEHCOL)                                            \
  ((type.r == VEHCOL.r) && (type.g == VEHCOL.g) && (type.b == VEHCOL.b))

void IVFCarcols::Initialize() { m_bEnabled = true; }

bool IVFCarcols::GetColor(CVehicle *pVeh, RpMaterial *pMat, CRGBA &col) {
  CRGBA *colorTable = *reinterpret_cast<CRGBA **>(0x4C8390);
  CRGBA type = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
  type.a == 255;

  int model = pVeh->m_nModelIndex;
  if (m_bEnabled && variations.contains(model)) {
    auto &data = ExData.Get(pVeh);
    int random = rand() % variations[model].size();
    auto storeCol = variations[model][random];

    if (type.r == VEHCOL_PRIMARY.r &&
        type.g == VEHCOL_PRIMARY.g) { // blue can be anything
      if (!data.m_bPri) {
        data.m_Colors.primary = storeCol.primary;
        data.m_bPri = true;
      }
      col = data.m_Colors.primary;
    } else if (IS_SAME_COLOR(type, VEHCOL_SECONDARY)) {
      if (!data.m_bSec) {
        data.m_Colors.secondary = storeCol.secondary;
        data.m_bSec = true;
      }
      col = data.m_Colors.secondary;
    } else if (IS_SAME_COLOR(type, VEHCOL_TERTIARY)) {
      if (!data.m_bTer) {
        data.m_Colors.tert = storeCol.tert;
        data.m_bTer = true;
      }
      col = data.m_Colors.tert;
    } else if (IS_SAME_COLOR(type, VEHCOL_QUATARNARY)) {
      if (!data.m_bQuat) {
        data.m_Colors.quart = storeCol.quart;
        data.m_bQuat = true;
      }
      col = data.m_Colors.quart;
    } else {
      return false;
    }
  } else {
    int idx = 0;
    if (type.r == VEHCOL_PRIMARY.r &&
        type.g == VEHCOL_PRIMARY.g) { // blue can be anything
      idx = CVehicleModelInfo::ms_currentCol[0];
    } else if (IS_SAME_COLOR(type, VEHCOL_SECONDARY)) {
      idx = CVehicleModelInfo::ms_currentCol[1];
    } else if (IS_SAME_COLOR(type, VEHCOL_TERTIARY)) {
      idx = CVehicleModelInfo::ms_currentCol[2];
    } else if (IS_SAME_COLOR(type, VEHCOL_QUATARNARY)) {
      idx = CVehicleModelInfo::ms_currentCol[3];
    } else {
      return false;
    }
    col = colorTable[idx];
  }

  return true;
}

void IVFCarcols::Parse(const nlohmann::json &data, int model) {
  if (data.contains("carcols")) {
    auto &cols = data["carcols"]["colors"];
    auto &var = data["carcols"]["variations"];

    for (auto &e : var) {
      int pIdx = e.value("primary", 0);
      int sIdx = e.value("secondary", 0);
      int tIdx = e.value("tertiary", 0);
      int qIdx = e.value("quaternary", 0);

      auto maxIdx = cols.size();
      if (pIdx >= maxIdx || sIdx >= maxIdx || tIdx >= maxIdx ||
          qIdx >= maxIdx) {
        gLogger->error(
            "Carcols index out of bounds for model '{}': "
            "primary={}, secondary={}, tertiary={}, quaternary={}, max={}",
            model, pIdx, sIdx, tIdx, qIdx, maxIdx);
        continue;
      }

      auto &pCol = cols.at(pIdx);
      auto &sCol = cols.at(sIdx);
      auto &tCol = cols.at(tIdx);
      auto &qCol = cols.at(qIdx);

      CRGBA primaryColor = CRGBA(pCol["red"], pCol["green"], pCol["blue"], 255);
      CRGBA secondaryColor =
          CRGBA(sCol["red"], sCol["green"], sCol["blue"], 255);
      CRGBA tertiaryColor =
          CRGBA(tCol["red"], tCol["green"], tCol["blue"], 255);
      CRGBA quaternaryColor =
          CRGBA(qCol["red"], qCol["green"], qCol["blue"], 255);

      variations[model].push_back(
          {primaryColor, secondaryColor, tertiaryColor, quaternaryColor});
    }
  }
}