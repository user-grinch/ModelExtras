#include "dummy.h"
#include "datamgr.h"
#include "defines.h"
#include "enums/dummypos.h"
#include "pch.h"
#include <CWorld.h>

extern float gfGlobalCoronaSize;
extern int gGlobalCoronaIntensity;
extern int gGlobalShadowIntensity;

int ReadHex(char a, char b) {
  a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
  b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

  return (a << 4) + b;
}

VehicleDummy::VehicleDummy(const VehicleDummyConfig &config) {
  data = config;
  float angleVal = 0.0f;

  // Calculate the angle based on the frame's orientation
  data.rotation.angle = Util::RadToDeg(CGeneral::GetATanOfXY(
      data.frame->modelling.right.x, data.frame->modelling.right.y));

  auto &jsonData = DataMgr::Get(data.pVeh->m_nModelIndex);
  std::string name = GetFrameNodeName(data.frame);

  if (jsonData.contains("lights")) {
    std::string newName = name.substr(0, name.find("_prm"));
    if (jsonData["lights"].contains(newName.c_str())) {
      auto &lights = jsonData["lights"][newName.c_str()];

      if (lights.contains("corona")) {
        auto &coronaSec = lights["corona"];
        if (coronaSec.contains("color")) {
          data.corona.color.r =
              coronaSec["color"].value("red", data.corona.color.r);
          data.corona.color.g =
              coronaSec["color"].value("green", data.corona.color.g);
          data.corona.color.b =
              coronaSec["color"].value("blue", data.corona.color.b);
          data.corona.color.a =
              coronaSec["color"].value("alpha", gGlobalCoronaIntensity);
        }
        data.corona.size = coronaSec.value("size", gfGlobalCoronaSize);
        data.corona.lightingType =
            GetLightingMode(coronaSec.value("type", "directional"));
      }

      if (lights.contains("shadow")) {
        auto &shadow = lights["shadow"];
        if (shadow.contains("color")) {
          data.shadow.color.r =
              shadow["color"].value("red", data.shadow.color.r);
          data.shadow.color.g =
              shadow["color"].value("green", data.shadow.color.g);
          data.shadow.color.b =
              shadow["color"].value("blue", data.shadow.color.b);
          data.shadow.color.a =
              shadow["color"].value("alpha", gGlobalShadowIntensity);
        }
        data.shadow.size = shadow.value("size", 1.0f);
        data.shadow.texture = shadow.value("texture", "");
        data.shadow.rotationChecks = shadow.value("rotationchecks", true);

        // shadows will be force enabled if there is JSON data for it.
        data.shadow.render = true;
      }

      // Only for StrobeLights
      if (lights.contains("strobedelay")) {
        data.strobe.delay = lights.value("strobedelay", 1000);
      }
    }
  } else {
    // Legacy support for ImVehFt vehicles
    size_t prmPos = name.find("_prm");
    if (prmPos != std::string::npos) {
      if (prmPos + 9 < name.size()) {
        data.shadow.color.r = data.corona.color.r =
            ReadHex(name[prmPos + 4], name[prmPos + 5]);
        data.shadow.color.g = data.corona.color.g =
            ReadHex(name[prmPos + 6], name[prmPos + 7]);
        data.shadow.color.b = data.corona.color.b =
            ReadHex(name[prmPos + 8], name[prmPos + 9]);
      } else {
        LOG_VERBOSE("Model {} has issue with node `{}`: invalid color format",
                    data.pVeh->m_nModelIndex, name);
      }

      if (prmPos + 10 < name.size()) {
        int type = name[prmPos + 10] - '0';
        data.corona.lightingType = (type == 2) ? eLightingMode::NonDirectional
                                               : eLightingMode::Directional;
      } else {
        data.corona.lightingType = eLightingMode::NonDirectional;
        LOG_VERBOSE("Model {} has issue with node `{}`: invalid light type",
                    data.pVeh->m_nModelIndex, name);
      }

      if (prmPos + 11 < name.size()) {
        data.corona.size = static_cast<float>(name[prmPos + 11] - '0') / 10.0f;
        if (data.corona.size < 0.0f) {
          data.corona.size = 0.0f;
        }
      } else {
        data.corona.size = 0.0f;
        LOG_VERBOSE("Model {} has issue with node `{}`: invalid corona size",
                    data.pVeh->m_nModelIndex, name);
      }

      if (prmPos + 12 < name.size()) {
        float shadowValue = static_cast<float>(name[prmPos + 12] - '0') / 7.5f;
        data.shadow.size = std::max(shadowValue, 0.0f);
      } else {
        data.shadow.size = 0.0f;
        LOG_VERBOSE("Model {} has issue with node `{}`: invalid shadow size",
                    data.pVeh->m_nModelIndex, name);
      }
    }
  }
}

void VehicleDummy::Update() {
  CMatrix &vehMatrix = *(CMatrix *)data.pVeh->GetMatrix();
  CVector pos = data.pVeh->GetPosition();
  CVector dummyPos = data.frame->ltm.pos;
  CVector offset = dummyPos - pos;

  // Transform to local space using  transpose of the rotation matrix
  data.shadow.position.x = data.position.x = vehMatrix.right.x * offset.x +
                                             vehMatrix.right.y * offset.y +
                                             vehMatrix.right.z * offset.z;
  data.shadow.position.y = data.position.y = vehMatrix.up.x * offset.x +
                                             vehMatrix.up.y * offset.y +
                                             vehMatrix.up.z * offset.z;
  data.shadow.position.z = data.position.z = vehMatrix.at.x * offset.x +
                                             vehMatrix.at.y * offset.y +
                                             vehMatrix.at.z * offset.z;

  if (data.mirroredX) {
    data.position.x *= -1;
    data.shadow.position.x *= -1;
  }
}