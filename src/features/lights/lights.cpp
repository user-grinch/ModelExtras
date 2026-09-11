#include "enums/materialtype.h"
#include "pch.h"
#include "lights.h"
#include "manager.h"
#include "utils/meevents.h"
#include "utils/datamgr.h"
#include "ModelExtrasAPI.h"

float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;
bool gbLightPointLights = true;
bool gbSirenPointLights = false;

void Lights::Init() {
    ReloadConfig();
    if (!m_bEnabled) {
        return;
    }

    LightManager::Init();

    patch::Nop(0x6E2722, 19);	  // CVehicle::DoHeadLightReflection
	patch::SetUChar(0x6E1A22, 0); // CVehicle::DoTailLightEffect

	// CVehicle::DoHeadLightEffect
	patch::SetUChar(0x6E0CF8, 0);
	patch::SetUChar(0x6E0DEE, 0);

	// NOP CVehicle::DoHeadLightBeam
	if (!gConfig.ReadBoolean("LIGHTS", "HeadLightBeams", gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true)))
	{
		// cmp ax, ax
		patch::SetRaw(0x6A2EA5, (void *)"\x66\x39\xC0\x90", 4);
		patch::SetRaw(0x6BDE63, (void *)"\x66\x39\xC0\x90\x90\x90\x90", 7);
	}

	Events::initGameEvent += []()
	{
		LightsConfig::Get().InitConfig();
	};

    ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat) {
        if (!m_bEnabled) return eMaterialType::UnknownMaterial;
        return LightManager::GetMatType(pMat); 
    });

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName) {
        LightManager::RegisterDummy(pVeh, pFrame, nodeName);
    });

    ModelInfoMgr::RegisterMaterialColProvider([](CVehicle *pVeh, RpMaterial *pMat, eMaterialType type) -> MatStateColor {
        if (!m_bEnabled || !pVeh || type < 0 || type >= eMaterialType::TotalMaterial) {
            return MatStateColor{DEFAULT_MAT_COL, DEFAULT_MAT_COL};
        }
        VehLightData &data = LightManager::m_VehData.Get(pVeh);
        if (LightManager::IsDummyAvailable(data, type)) {
            const DummyConfig &c = data.dummies[type][0]->GetRef();
            if (c.hasCustomColor) {
                return MatStateColor{c.corona.color, DEFAULT_MAT_COL};
            }
        }

        auto &json = DataMgr::Get(pVeh->m_nModelIndex);
        if (json.contains("lights")) {
            auto &lights = json["lights"];
            auto CheckCol = [&](const char *key) -> std::optional<CRGBA> {
                if (lights.contains(key) && lights[key].contains("corona") && lights[key]["corona"].contains("color")) {
                    auto &c = lights[key]["corona"]["color"];
                    CRGBA col;
                    col.r = c.value("red", 255);
                    col.g = c.value("green", 255);
                    col.b = c.value("blue", 255);
                    col.a = 255;
                    return col;
                }
                return std::nullopt;
            };

            std::optional<CRGBA> col;
            switch (type) {
            case eMaterialType::HeadLightLeft: col = CheckCol("headlight_l"); if (!col) col = CheckCol("headlights"); break;
            case eMaterialType::HeadLightRight: col = CheckCol("headlight_r"); if (!col) col = CheckCol("headlights"); break;
            case eMaterialType::TailLightLeft: col = CheckCol("taillight_l"); if (!col) col = CheckCol("taillights"); break;
            case eMaterialType::TailLightRight: col = CheckCol("taillight_r"); if (!col) col = CheckCol("taillights"); break;
            case eMaterialType::BrakeLightLeft: case eMaterialType::NABrakeLightLeft: col = CheckCol("brakelight_l"); if (!col) col = CheckCol("brakelights"); break;
            case eMaterialType::BrakeLightRight: case eMaterialType::NABrakeLightRight: col = CheckCol("brakelight_r"); if (!col) col = CheckCol("brakelights"); break;
            case eMaterialType::ReverseLightLeft: col = CheckCol("reverselight_l"); if (!col) col = CheckCol("reverselights"); break;
            case eMaterialType::ReverseLightRight: col = CheckCol("reverselight_r"); if (!col) col = CheckCol("reverselights"); break;
            case eMaterialType::IndicatorLightLeftFront: col = CheckCol("indicator_lf"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::IndicatorLightRightFront: col = CheckCol("indicator_rf"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::IndicatorLightLeftRear: col = CheckCol("indicator_lr"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::IndicatorLightRightRear: col = CheckCol("indicator_rr"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::IndicatorLightLeftMiddle: col = CheckCol("indicator_lm"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::IndicatorLightRightMiddle: col = CheckCol("indicator_rm"); if (!col) col = CheckCol("indicators"); break;
            case eMaterialType::FogLightLeft: col = CheckCol("foglight_l"); if (!col) col = CheckCol("fogl_l"); if (!col) col = CheckCol("foglights"); break;
            case eMaterialType::FogLightRight: col = CheckCol("foglight_r"); if (!col) col = CheckCol("fogl_r"); if (!col) col = CheckCol("foglights"); break;
            default: break;
            }
            if (col) {
                return MatStateColor{*col, DEFAULT_MAT_COL};
            }
        }

        return MatStateColor{DEFAULT_MAT_COL, DEFAULT_MAT_COL};
    });

	MEEvents::vehPreRenderEvent.before += [](CVehicle *pVeh)
	{
		if (!m_bEnabled) return;
		LightManager::ProcessPointLights(pVeh);
	};



	ModelInfoMgr::RegisterRender([](CVehicle *pControlVeh) {
		if (!m_bEnabled) return;
		int model = pControlVeh->m_nModelIndex;

		if (CModelInfo::IsTrailerModel(model)) {
			return;
		}

		CVehicle *pTowedVeh = pControlVeh;
		if (pControlVeh->m_pTrailer) {
			pTowedVeh = pControlVeh->m_pTrailer;
		}

		LightManager::Render(pControlVeh, pTowedVeh);
	});
}

void Lights::ReloadConfig() {
	CBaseFeature::ReloadConfig();
	if (!m_bActive) {
		m_bActive = gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false));
	}
	m_bEnabled = m_bActive;
	LightsConfig::Get().InitConfig();
}

void Lights::Reload(CVehicle* pVeh) {
	ReloadConfig();
	LightManager::Reload(pVeh);
}

VehLightData& Lights::GetVehicleData(CVehicle* pVeh) {
    return LightManager::m_VehData.Get(pVeh);
}

bool Lights::IsIndicatorOn(CVehicle* pVeh) {
    return LightManager::IsIndicatorOn(pVeh);
}

bool Lights::GetLightState(CVehicle* pVeh, eMaterialType lightId) {
    return LightManager::GetLightState(pVeh, lightId);
}

void Lights::SetLightState(CVehicle* pVeh, eMaterialType lightId, bool state) {
    LightManager::SetLightState(pVeh, lightId, state);
}

extern "C"
{
	ME_WRAPPER bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId)
	{
		return LightManager::GetLightState(pVeh, static_cast<eMaterialType>(lightId));
	}

	ME_WRAPPER void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state)
	{
		LightManager::SetLightState(pVeh, static_cast<eMaterialType>(lightId), state);
	}

	// Dummy function to show on crash logs
	int __declspec(dllexport) ignore4(int i)
	{
		return 1;
	}
}

void Lights::ProcessTick() {
    if (!m_bEnabled) return;
    BlinkerState::Get().Update();
}

void Lights::ProcessVehicle(CVehicle* pVeh) {
    if (!m_bEnabled) return;
    LightManager::Process(pVeh);
}
