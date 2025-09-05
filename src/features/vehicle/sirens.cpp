#include "sirens.h"
#include "audiomgr.h"
#include "datamgr.h"
#include "defines.h"
#include "enums/lighttype.h"
#include "lights.h"
#include "pch.h"
#include "util.h"
#include <CPointLights.h>
#include <CShadows.h>
#include <common.h>
#include <rpworld.h>
#include <rwcore.h>

bool VehicleSiren::GetSirenState() {
  return (Mute == false) ? (vehicle->bSirenOrAlarm) : (true);
};

char __fastcall Sirens::hkUsesSiren(CVehicle *ptr) {
  if (Util::IsEngineOff(ptr)) {
    ptr->bSirenOrAlarm = false;
    return false;
  }

  if (Sirens::modelData.contains(ptr->m_nModelIndex)) {
    ptr->m_vehicleAudio.m_bModelWithSiren = true;
    return true;
  }
  return ptr->IsLawEnforcementVehicle();
}

static CVehicle *pCurrentVeh = nullptr;
void __fastcall hkVehiclePreRender(CVehicle *ptr) {
  pCurrentVeh = ptr;
  plugin::CallMethod<0x6D6480>(ptr);
}

void Sirens::Reload(CVehicle *pVeh) {
  int model = pVeh->m_nModelIndex;
  modelRotators.erase(model);
  modelData.erase(model);
  vehicleData.erase(pVeh);
  EventCtor(pVeh);
  DataMgr::Reload(model);
}

void __cdecl Sirens::hkAddPointLights(uint8_t type, CVector point, CVector dir,
                                      float range, float red, float green,
                                      float blue, uint8_t fogEffect,
                                      bool bCastsShadowFromPlayerCarAndPed,
                                      CEntity *castingEntity) {
  if (!pCurrentVeh || !Sirens::modelData.contains(pCurrentVeh->m_nModelIndex)) {
    CPointLights::AddLight(type, point, dir, range, red, green, blue, fogEffect,
                           bCastsShadowFromPlayerCarAndPed, castingEntity);
  }
}

VehicleSirenMaterial::VehicleSirenMaterial(std::string state, int material,
                                           nlohmann::json json) {
  if (json.size() == 0) {
    LOG_VERBOSE("Model {} siren configuration exception!",
                Sirens::CurrentModel);
    return;
  }

  if (json.contains("reference")) {
    std::vector<std::string> references;

    if (json["reference"].is_string())
      references.push_back(json["reference"]);
    else if (json["reference"].is_array()) {
      for (nlohmann::json::iterator reference = json["reference"].begin();
           reference != json["reference"].end(); ++reference) {
        if (reference.value().is_string())
          references.push_back(reference.value());
        else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", reference array property is not an string!");
      }
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", reference property is not an array or string!");

    if (references.size() != 0) {
      for (std::vector<std::string>::iterator ref = references.begin();
           ref != references.end(); ++ref) {
        if (VehicleSirenData::References.contains((*ref))) {
          for (nlohmann::json::iterator reference =
                   VehicleSirenData::References[(*ref)].begin();
               reference != VehicleSirenData::References[(*ref)].end();
               ++reference) {
            if (json.contains(reference.key())) {
              if (json[reference.key()].is_object()) {
                nlohmann::json object = reference.value();

                for (nlohmann::json::iterator objectReference = object.begin();
                     objectReference != object.end(); ++objectReference) {
                  if (json[reference.key()].contains(objectReference.key()))
                    continue;

                  json[reference.key()][objectReference.key()] =
                      objectReference.value();
                }
              }

              continue;
            }

            json[reference.key()] = reference.value();
          }
        } else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", reference array property '" + (*ref) +
                      "' is not an recognized reference!");
      }
    }
  }

  if (json.contains("size")) {
    if (json["size"].is_number())
      Size = json["size"];
    else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", size property is not an acceptable number!");
  }

  if (json.contains("diffuse")) {
    if (json["diffuse"].is_boolean())
      Diffuse.Color = json["diffuse"];
    else if (json["diffuse"].is_object()) {
      if (json["diffuse"].contains("color")) {
        if (json["diffuse"]["color"].is_boolean())
          Diffuse.Color = json["diffuse"]["color"];
        else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", diffuse property color is not an boolean!");
      }

      if (json["diffuse"].contains("transparent")) {
        if (json["diffuse"]["transparent"].is_boolean())
          Diffuse.Transparent = json["diffuse"]["transparent"];
        else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", diffuse property transparent is not an boolean!");
      }
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", diffuse property is not an boolean or object!");
  }

  if (json.contains("radius")) {
    if (json["radius"].is_number())
      Radius = json["radius"];
    else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", radius property is not an acceptable number!");
  }

  if (json.contains("color")) {
    if (json["color"].is_string() &&
        VehicleSirenData::ReferenceColors.contains(json["color"]))
      json["color"] = VehicleSirenData::ReferenceColors[json["color"]];

    if (json["color"].is_object()) {
      if (json["color"].contains("red"))
        Color.r = json["color"]["red"];

      if (json["color"].contains("green"))
        Color.g = json["color"]["green"];

      if (json["color"].contains("blue"))
        Color.b = json["color"]["blue"];

      if (json["color"].contains("alpha"))
        Color.a = json["color"]["alpha"];

      DefaultColor = Color;
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", color property is not an object!");
  }

  if (json.contains("state")) {
    if (json["state"].is_boolean() || json["state"].is_number()) {
      bool state = (json["state"].is_boolean())
                       ? ((bool)json["state"])
                       : (((json["state"] == 0) ? (false) : (true)));

      State = state;

      StateDefault = state;
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", state property is not an boolean or number!");
  }

  if (json.contains("colors")) {
    if (json["colors"].is_array()) {
      for (nlohmann::json::iterator pattern = json["colors"].begin();
           pattern != json["colors"].end(); ++pattern) {
        nlohmann::json value = pattern.value();

        if (!value.is_array()) {
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", colors array property is not an array!");

          continue;
        }

        if (value.size() != 2) {
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", colors array property is not an array with a pair "
                      "value of delay and color!");

          continue;
        }

        uint64_t time = value[0];

        RwRGBA color;

        if (value[1].is_string() &&
            VehicleSirenData::ReferenceColors.contains(value[1]))
          value[1] = VehicleSirenData::ReferenceColors[value[1]];

        if (value[1].is_object()) {
          if (value[1].contains("red"))
            color.red = value[1]["red"];

          if (value[1].contains("green"))
            color.green = value[1]["green"];

          if (value[1].contains("blue"))
            color.blue = value[1]["blue"];

          if (value[1].contains("alpha"))
            color.alpha = value[1]["alpha"];

          Colors.push_back(std::make_pair(time, color));

          ColorTotal += time;
        } else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", colors array property does not contain an array!");
      }
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", colors property is not an array!");
  }

  if (json.contains("pattern")) {
    if (json["pattern"].is_array()) {
      for (nlohmann::json::iterator pattern = json["pattern"].begin();
           pattern != json["pattern"].end(); ++pattern) {
        nlohmann::json value = pattern.value();

        if (value.is_array()) {
          if (value.size() < 2) {
            LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                        " siren configuration exception!\n \nState '" + state +
                        "' material " + std::to_string(material) +
                        ", pattern array property is an array but does not "
                        "contain an iteration and pattern setting!");

            continue;
          }

          for (int count = 0; count < value[0]; count++) {
            for (nlohmann::json::iterator vector = ++value.begin();
                 vector != value.end(); ++vector) {
              int time = vector.value();

              Pattern.push_back(time);

              PatternTotal += time;
            }
          }

          continue;
        } else if (value.is_number()) {
          int time = pattern.value();

          Pattern.push_back(time);

          PatternTotal += time;
        } else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", pattern array property is not an array or number!");
      }
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", pattern property is not an array!");
  }

  if (json.contains("type")) {
    if (json["type"].is_string()) {
      Type = GetLightingMode(json["type"]);
      if (Type == eLightingMode::Rotator && json.contains("rotator")) {
        if (json["rotator"].is_object())
          Rotator = new VehicleSirenRotator(json["rotator"]);
        else
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", rotator property is not an object!");
      }
    } else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", type property is not a string!");
  }

  if (json.contains("shadow")) {
    if (json["shadow"].is_object()) {
      if (json["shadow"].contains("angleoffset")) {
        if (json["shadow"]["angleoffset"].is_number()) {
          Shadow.AngleOffset = json["shadow"]["angleoffset"];
        } else {
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", shadow object property type is not an acceptable "
                      "number or string!");
        }
      }

      if (json["shadow"].contains("type")) {
        if (json["shadow"]["type"].is_string()) {
          Shadow.Type = json["shadow"]["type"];
        } else {
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " siren configuration exception!\n \nState '" + state +
                      "' material " + std::to_string(material) +
                      ", shadow object property type is not an acceptable "
                      "number or string!");
        }
      }

      if (json["shadow"].contains("size")) {
        if (json["shadow"]["size"].is_number()) {
          Shadow.Size = json["shadow"]["size"];
        } else {
          LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                      " has wrong shadow size. Using default value!");
        }
      }

      if (json["shadow"].contains("offset")) {
        if (json["shadow"]["offset"].is_number())
          Shadow.Offset = json["shadow"]["offset"];
        else
          LOG_VERBOSE(
              "Model " + std::to_string(Sirens::CurrentModel) +
              " siren configuration exception!\n \nState '" + state +
              "' material " + std::to_string(material) +
              ", shadow object property offset is not an acceptable number!");
      }
    } else {
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", shadow property is not an object!");
    }
  }

  if (json.contains("delay")) {
    if (json["delay"].is_number())
      Delay = json["delay"];
    else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", delay property is not an acceptable number!");
  }

  if (json.contains("inertia")) {
    if (json["inertia"].is_number())
      Inertia = json["inertia"];
    else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", inertia property is not an acceptable number!");
  }

  if (json.contains("imvehft")) {
    if (json["imvehft"].is_boolean() || json["imvehft"].is_number())
      ImVehFt = json["imvehft"];
    else
      LOG_VERBOSE("Model " + std::to_string(Sirens::CurrentModel) +
                  " siren configuration exception!\n \nState '" + state +
                  "' material " + std::to_string(material) +
                  ", ImVehFt property is not a boolean or number!");
  }

  Validate = true;
};

VehicleSirenState::VehicleSirenState(std::string state, nlohmann::json json) {
  Name = state;

  Paintjob = -1;

  if (json.size() == 0) {
    LOG_VERBOSE("Failed to set up state " + state +
                ", could not find any keys in the manifest!\n");

    return;
  }

  for (nlohmann::json::iterator material = json.begin(); material != json.end();
       ++material) {
    if (material.key() == "paintjob") {
      Paintjob = material.value();

      continue;
    }

    int materialIndex = std::stoi(material.key());

    Materials[materialIndex] =
        new VehicleSirenMaterial(state, materialIndex, material.value());

    if (!Materials[materialIndex]->Validate) {
      Materials.erase(materialIndex);

      LOG_VERBOSE("Failed to set up state " + state + "'s material " +
                  material.key() +
                  ", could not configure object from manifest!\n");

      continue;
    }
  }

  Validate = true;
};

VehicleSirenData::VehicleSirenData(nlohmann::json json) {
  if (json.size() == 0) {
    LOG_VERBOSE(
        "Failed to set up states, could not find any keys in the manifest!\n");
    return;
  }

  VehicleSirenData::References.clear();
  VehicleSirenData::ReferenceColors.clear();

  nlohmann::json states = json;

  if (json.contains("states")) {
    states = json["states"];

    if (json.contains("references")) {
      nlohmann::json references = json["references"];

      for (nlohmann::json::iterator reference = references.begin();
           reference != references.end(); ++reference) {
        std::string key = reference.key();

        nlohmann::json value = reference.value();

        if (key == "colors") {
          for (nlohmann::json::iterator color = value.begin();
               color != value.end(); ++color) {
            VehicleSirenData::ReferenceColors[color.key()] = color.value();
          }

          continue;
        }

        VehicleSirenData::References[key] = value;
      }
    }
  }

  for (nlohmann::json::iterator stateObject = states.begin();
       stateObject != states.end(); ++stateObject) {
    VehicleSirenState *state =
        new VehicleSirenState(stateObject.key(), stateObject.value());

    if (!state->Validate) {
      LOG_VERBOSE("Failed to set up state " + stateObject.key() +
                  ", could not configure object from manifest!\n");

      continue;
    }

    States.push_back(state);
  }

  Validate = true;
};

void Sirens::Parse(const nlohmann::json &data, int model) {
  if (data.contains("sirens")) {
    CurrentModel = model;
    modelData[CurrentModel] = new VehicleSirenData(data["sirens"]);
    modelData[CurrentModel]->isImVehFtSiren =
        data["sirens"].contains("imvehft") && data["sirens"]["imvehft"];

    if (!modelData[CurrentModel]->Validate) {
      LOG_VERBOSE("Failed to read siren configuration, cannot configure JSON "
                  "manifest!");
      modelData.erase(CurrentModel);
    }
  }
}

void Sirens::EventCtor(CVehicle *pVeh) {
  if (modelData.contains(pVeh->m_nModelIndex)) {
    vehicleData[pVeh] = new VehicleSiren(pVeh);
  }
}

int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat) {
  int model = pVeh->m_nModelIndex;
  if (Sirens::modelData.contains(model)) {
    if (Sirens::modelData[model]->isImVehFtSiren) {
      return 256 - pMat->color.red; // 256 is correct, not 255
    } else {
      return pMat->color.red;
    }
  }
  return -1;
}

void Sirens::Initialize() {
  m_bEnabled = true;
  ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat) {
    if (!m_bEnabled) {
      return eLightType::UnknownLight;
    }
    CRGBA col = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(pMat));
    if (modelData.contains(pVeh->m_nModelIndex) &&
        (col.b == 255 || col.g == 255 ||
         modelData[pVeh->m_nModelIndex]->isImVehFtSiren)) {
      if (modelData[pVeh->m_nModelIndex]->isImVehFtSiren && pMat->texture) {
        if ((std::string(pMat->texture->name).find("siren", 0) != 0 ||
             std::string(pMat->texture->name).find("vehiclelights128", 0) !=
                 0) &&
            (col.r >= 240 && col.g == 0 && col.b == 0)) {
          return eLightType::SirenLight;
        }
      } else if (col.r > 0 && col.g == 255 && col.b == 255) {
        return eLightType::SirenLight;
      }
    }
    return eLightType::UnknownLight;
  });

  ModelInfoMgr::RegisterMaterialColProvider(
      [](CVehicle *pVeh, RpMaterial *pMat, eLightType type) {
        if (type == eLightType::SirenLight) {
          int matIdx = GetSirenIndex(pVeh, pMat);

          if (matIdx != -1) {
            int curState = vehicleData[pVeh]->GetCurrentState();
            auto &state = modelData[pVeh->m_nModelIndex]->States[curState];
            if (state->Materials.contains(matIdx)) {
              if (modelData[pVeh->m_nModelIndex]->isImVehFtSiren) {
                return MatStateColor{state->Materials[matIdx]->Color,
                                     state->Materials[matIdx]->Color};
              } else {
                return MatStateColor{state->Materials[matIdx]->Color,
                                     DEFAULT_MAT_COL};
              }
            }
          }
        }
        return MatStateColor{DEFAULT_MAT_COL, DEFAULT_MAT_COL};
      });

  ModelInfoMgr::RegisterDummy([](CVehicle *vehicle, RwFrame *frame) {
    std::string name = GetFrameNodeName(frame);
    if (!modelData.contains(vehicle->m_nModelIndex)) {
      return;
    }

    if (!vehicleData.contains(vehicle)) {
      vehicleData[vehicle] = new VehicleSiren(vehicle);
    }

    int id = Util::GetDigitsAfter(name, "siren_").value_or(-1);
    id = Util::GetDigitsAfter(name, "siren").value_or(id);
    id = Util::GetDigitsAfter(name, "light_em").value_or(id);

    if (id != -1) {
      VehicleDummyConfig config{
          .pVeh = vehicle,
          .frame = frame,
      };
      vehicleData[vehicle]->Dummies[id].push_back(new VehicleDummy(config));
    }
  });

  plugin::Events::vehicleCtorEvent += [](CVehicle *pVeh) { EventCtor(pVeh); };

  plugin::Events::vehicleDtorEvent += [](CVehicle *vehicle) {
    int model = vehicle->m_nModelIndex;

    if (!modelData.contains(model))
      return;

    vehicleData.erase(vehicle);
  };

  Events::processScriptsEvent += []() {
    CVehicle *vehicle = FindPlayerVehicle(-1, false);
    if (!vehicle ||
        vehicle->m_nOverrideLights == eLightOverride::ForceLightsOff) {
      return;
    }

    static size_t prev = 0;
    size_t now = CTimer::m_snTimeInMilliseconds;

    if (now - prev > 300.0f) {
      static uint32_t sirenKey =
          gConfig.ReadInteger("KEYS", "SirenLightKey", VK_L);
      if (KeyPressed(sirenKey)) {
        int model = vehicle->m_nModelIndex;

        if (!modelData.contains(model))
          return;

        if (!vehicleData.contains(vehicle))
          return;

        vehicleData[vehicle]->Mute = !vehicleData[vehicle]->Mute;

        if (vehicleData[vehicle]->Mute)
          vehicle->bSirenOrAlarm = false;

        AudioMgr::PlaySwitchSound(vehicle);
      }

      if (KeyPressed(VK_R)) {
        int model = vehicle->m_nModelIndex;

        if (!modelData.contains(model))
          return;

        if (!vehicleData.contains(vehicle))
          return;

        if (!vehicleData[vehicle]->GetSirenState())
          return;

        if (modelData[model]->States.size() == 0)
          return;

        int addition = (plugin::KeyPressed(0x10)) ? (-1) : (1);

        vehicleData[vehicle]->State += addition;

        if (vehicleData[vehicle]->State == modelData[model]->States.size())
          vehicleData[vehicle]->State = 0;

        if (vehicleData[vehicle]->State == -1)
          vehicleData[vehicle]->State = modelData[model]->States.size() - 1;

        while (
            modelData[model]->States[vehicleData[vehicle]->State]->Paintjob !=
                -1 &&
            modelData[model]->States[vehicleData[vehicle]->State]->Paintjob !=
                vehicle->GetRemapIndex()) {
          vehicleData[vehicle]->State +=
              (plugin::KeyPressed(0x10)) ? (-1) : (1);

          if (vehicleData[vehicle]->State == modelData[model]->States.size()) {
            vehicleData[vehicle]->State = 0;

            break;
          } else if (vehicleData[vehicle]->State == -1) {
            vehicleData[vehicle]->State = modelData[model]->States.size() - 1;

            break;
          }
        }
      }
      prev = now;
    }
    for (int number = 0; number < 9; number++) {
      if (KeyPressed(VK_1 + number)) { // 1 -> 9
        int model = vehicle->m_nModelIndex;

        if (!modelData.contains(model))
          return;

        if (!vehicleData.contains(vehicle))
          return;

        if (!vehicleData[vehicle]->GetSirenState())
          return;

        if (modelData[model]->States.size() == 0)
          return;

        int newState = number;

        if ((int)modelData[model]->States.size() <= newState)
          return;

        if (vehicleData[vehicle]->State == newState)
          return;

        if (modelData[model]->States[newState]->Paintjob != -1 &&
            modelData[model]->States[newState]->Paintjob !=
                vehicle->GetRemapIndex())
          return;

        vehicleData[vehicle]->State = newState;
      }
    }
  };

  ModelInfoMgr::RegisterRender([](CVehicle *vehicle) {
    int model = vehicle->m_nModelIndex;

    if (!vehicle->GetIsOnScreen() || !modelData.contains(model) ||
        vehicle->m_nOverrideLights == eLightOverride::ForceLightsOff ||
        vehicle->ms_forceVehicleLightsOff) {
      return;
    }

    if (Util::IsEngineOff(vehicle)) {
      vehicle->bSirenOrAlarm = false;
      return;
    }

    bool sirenState = vehicleData[vehicle]->GetSirenState();
    if (modelRotators.contains(model)) {
      for (auto &dummy : modelRotators[model]) {
        dummy->ResetAngle();
      }
      modelRotators.erase(model);
    }

    if (!vehicleData.contains(vehicle))
      vehicleData[vehicle] = new VehicleSiren(vehicle);

    uint64_t time = duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    VehicleSirenState *state =
        modelData[model]->States[vehicleData[vehicle]->GetCurrentState()];
    if (vehicleData[vehicle]->SirenState == false && sirenState == true) {
      vehicleData[vehicle]->SirenState = true;

      if (vehicleData[vehicle]->Delay != 0) {
        vehicleData[vehicle]->Delay = 0;
      }

      for (auto &mat : state->Materials) {
        mat.second->ColorTime = time;
        mat.second->PatternTime = time;
      }
    } else if (vehicleData[vehicle]->SirenState == true &&
               sirenState == false) {
      vehicleData[vehicle]->SirenState = false;
    }

    if (!vehicleData[vehicle]->GetSirenState() &&
        !vehicleData[vehicle]->Trailer) {
      return;
    }

    if (vehicleData[vehicle]->Delay == 0) {
      vehicleData[vehicle]->Delay = time;
    }

    for (auto &mat : state->Materials) {
      if (mat.second->Delay != 0) {
        if (time - vehicleData[vehicle]->Delay < mat.second->Delay) {
          if (mat.second->Type == eLightingMode::Rotator) {
            if ((time - mat.second->Rotator->TimeElapse) >
                mat.second->Rotator->Time) {
              mat.second->Rotator->TimeElapse = time;

              mat.second->ResetColor(time);

              if (mat.second->Rotator->Direction == 2) {
                mat.second->Rotator->Direction = 3;
              } else if (mat.second->Rotator->Direction == 3) {
                mat.second->Rotator->Direction = 2;
              }
            }
          }
          continue;
        }
      }

      if (mat.second->ColorTotal != 0) {
        if ((time - mat.second->ColorTime) >=
            mat.second->Colors[mat.second->ColorCount].first) {
          mat.second->ColorTime = time;
          RwRGBA color = mat.second->Colors[mat.second->ColorCount].second;
          mat.second->Color = {color.red, color.green, color.blue, color.alpha};
          mat.second->ColorCount++;

          if ((size_t)mat.second->ColorCount >= mat.second->Colors.size()) {
            mat.second->ColorCount = 0;
          }
        }
      }

      if (mat.second->UpdateMaterial(time)) {
        if (mat.second->PatternCount >= (int)mat.second->Pattern.size()) {
          for (std::map<int, VehicleSirenMaterial *>::iterator materialReset =
                   state->Materials.begin();
               materialReset != state->Materials.end(); ++materialReset) {
            if (mat.second->PatternTotal ==
                materialReset->second->PatternTotal) {
              materialReset->second->ResetMaterial(time);
            }
          }
        }
      } else if (mat.second->Type == eLightingMode::Rotator) {
        uint64_t elapsed = time - mat.second->Rotator->TimeElapse;
        if (elapsed > mat.second->Rotator->Time) {
          mat.second->Rotator->TimeElapse = time;
          mat.second->ResetColor(time);

          if (mat.second->Rotator->Direction == 2) {
            mat.second->Rotator->Direction = 3;
          } else if (mat.second->Rotator->Direction == 3) {
            mat.second->Rotator->Direction = 2;
          }
        }
      }
    }

    CVector distance = vehicle->GetPosition() - TheCamera.GetPosition();
    eCoronaFlareType type = FLARETYPE_NONE;

    if (distance.Magnitude() > 30.0f) {
      type = FLARETYPE_HEADLIGHTS;
    }

    for (auto &mat : state->Materials) {
      if (!mat.second->State) {
        continue;
      }

      if (mat.second->Delay != 0 &&
          time - vehicleData[vehicle]->Delay < mat.second->Delay) {
        continue;
      }

      if (mat.second->PatternTotal != 0 && mat.second->Inertia != 0.0f) {
        float currentTime = (float)(time - mat.second->PatternTime);
        float changeTime =
            (((float)mat.second->Pattern[mat.second->PatternCount]) / 2.0f) *
            mat.second->Inertia;
        float patternTotalTime =
            (float)mat.second->Pattern[mat.second->PatternCount];
        mat.second->InertiaMultiplier = 1.0f;

        if (currentTime < changeTime) {
          mat.second->InertiaMultiplier = (currentTime / changeTime);
        } else if (currentTime > (patternTotalTime - changeTime)) {
          currentTime = patternTotalTime - currentTime;
          mat.second->InertiaMultiplier = (currentTime / changeTime);
        }
      }

      int id = 0;
      for (auto &e : vehicleData[vehicle]->Dummies[mat.first]) {
        id++;
        EnableDummy((mat.first * 16) + id, e, vehicle, mat.second, type, time);
      }

      ModelInfoMgr::EnableSirenMaterial(vehicle, mat.first);
    }
  });

  patch::ReplaceFunctionCall(0x6D8492, hkUsesSiren);
  patch::ReplaceFunctionCall(0x6AB80F, hkAddPointLights);
  patch::ReplaceFunctionCall(0x6AAB71, hkVehiclePreRender);

  Events::initGameEvent += [] {
    injector::MakeCALL((void *)0x6ABA60, hkRegisterCorona, true);
    injector::MakeCALL((void *)0x6BD4DD, hkRegisterCorona, true);
    injector::MakeCALL((void *)0x6BD531, hkRegisterCorona, true);
  };
};

void Sirens::hkRegisterCorona(
    unsigned int id, CEntity *attachTo, unsigned char red, unsigned char green,
    unsigned char blue, unsigned char alpha, CVector const &posn, float radius,
    float farClip, eCoronaType coronaType, eCoronaFlareType flaretype,
    bool enableReflection, bool checkObstacles, int _param_not_used,
    float angle, bool longDistance, float nearClip, unsigned char fadeState,
    float fadeSpeed, bool onlyFromBelow, bool reflectionDelay) {
  CVehicle *vehicle = NULL;

  _asm {
		pushad
		mov vehicle, esi
		popad
  }

  if (vehicle && modelData.contains(vehicle->m_nModelIndex)) {
    return;
  }

  CCoronas::RegisterCorona(id, attachTo, red, green, blue, alpha, posn, radius,
                           farClip, coronaType, flaretype, enableReflection,
                           checkObstacles, _param_not_used, angle, longDistance,
                           nearClip, fadeState, fadeSpeed, onlyFromBelow,
                           reflectionDelay);
}

void Sirens::EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle,
                         VehicleSirenMaterial *material, eCoronaFlareType type,
                         uint64_t time) {
  dummy->Update();
  CVector position = reinterpret_cast<CVehicleModelInfo *>(
                         CModelInfo__ms_modelInfoPtrs[vehicle->m_nModelIndex])
                         ->m_pVehicleStruct->m_avDummyPos[0];
  unsigned char alpha = material->Color.a;

  if (material->PatternTotal != 0 && material->Inertia != 0.0f) {
    alpha = static_cast<char>(std::abs(alpha) * material->InertiaMultiplier);
  }

  VehicleDummyConfig *pDummyConfig = &dummy->Get();
  pDummyConfig->shadow.color = pDummyConfig->corona.color = material->Color;
  pDummyConfig->corona.size = material->Size;
  float dummyAngle = Util::NormalizeAngle(pDummyConfig->rotation.angle +
                                          material->Shadow.AngleOffset);

  if (material->Type != eLightingMode::NonDirectional) {
    if (material->Type == eLightingMode::Rotator) {
      uint64_t elapsed = time - material->Rotator->TimeElapse;

      float angle = ((elapsed / ((float)material->Rotator->Time)) *
                     material->Rotator->Radius);

      if (material->Rotator->Direction == 0)
        angle = 360.0f - angle;
      else if (material->Rotator->Direction == 2) {
        angle += material->Rotator->Offset;

        angle = 360.0f - angle;
      } else if (material->Rotator->Direction == 3) {
        angle += material->Rotator->Offset;
      }

      dummyAngle += angle;

      Sirens::modelRotators[vehicle->m_nModelIndex].push_back(dummy);

      dummy->SetAngle(angle);
      dummyAngle = Util::NormalizeAngle(dummyAngle);
    }
    RenderUtil::RegisterCoronaDirectional(
        pDummyConfig, dummyAngle, material->Radius, 1.0f,
        material->Type == eLightingMode::Inversed);
    RenderUtil::RegisterShadowDirectional(pDummyConfig, material->Shadow.Type,
                                          material->Shadow.Size);
  } else {
    RenderUtil::RegisterCorona(
        vehicle, (reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id,
        pDummyConfig->position, material->Color, material->Size);
    RenderUtil::RegisterShadow(
        vehicle, pDummyConfig->position, *(CRGBA *)&material->Color,
        dummyAngle + pDummyConfig->rotation.currentAngle,
        pDummyConfig->dummyType, material->Shadow.Type,
        {material->Shadow.Size, material->Shadow.Size},
        {material->Shadow.Offset, material->Shadow.Offset}, nullptr);
  }
};

VehicleSiren::VehicleSiren(CVehicle *_vehicle) {
  vehicle = _vehicle;

  int model = vehicle->m_nModelIndex;

  CVehicleModelInfo *modelInfo = reinterpret_cast<CVehicleModelInfo *>(
      CModelInfo__ms_modelInfoPtrs[model]);

  if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_HELI ||
      modelInfo->m_nVehicleType == eVehicleType::VEHICLE_PLANE)
    this->Mute = true;

  SirenState = _vehicle->bSirenOrAlarm;

  if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_TRAILER)
    Trailer = true;
};
