#include "enums/materialtype.h"
#include "pch.h"
#include "lights.h"
#include "manager.h"
#include "utils/meevents.h"

void LightsFeature::Init() {
    if (!gConfig.ReadBoolean("LIGHTS", "StandardLightsv2", gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false))) {
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
        return LightManager::GetMatType(pMat); 
    });

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName) {
        LightManager::RegisterDummy(pVeh, pFrame, nodeName);
    });

	MEEvents::vehPreRenderEvent.before += [](CVehicle *pVeh)
	{
		LightManager::ProcessPointLights(pVeh);
	};

	Events::processScriptsEvent += []()
	{
		BlinkerState::Get().Update();

		for (CVehicle *pVeh : CPools::ms_pVehiclePool)
		{
			if (pVeh) {
				if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
				{
					LightManager::ProcessPointLights(pVeh);
				}
				LightManager::Process(pVeh);
			}
		}
	};

	ModelInfoMgr::RegisterRender([](CVehicle *pControlVeh) {
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

void LightsFeature::ReloadConfig() {
	CBaseFeature::ReloadConfig();
	LightsConfig::Get().InitConfig();
}

void LightsFeature::Reload(CVehicle* pVeh) {
	ReloadConfig();
	LightManager::Reload(pVeh);
}