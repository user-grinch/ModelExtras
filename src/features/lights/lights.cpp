#include "enums/materialtype.h"
#include "pch.h"
#include "lights.h"
#include "manager.h"

void LightsFeature::Init() {
    if (!gConfig.ReadBoolean("FEATURES", "StandardLightsv2", false)) {
		return;
	}

    patch::Nop(0x6E2722, 19);	  // CVehicle::DoHeadLightReflection
	patch::SetUChar(0x6E1A22, 0); // CVehicle::DoTailLightEffect

	// // CVehicle::DoHeadLightEffect
	patch::SetUChar(0x6E0CF8, 0);
	patch::SetUChar(0x6E0DEE, 0);

	// NOP CVehicle::DoHeadLightBeam
	if (!gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true))
	{
		// cmp ax, ax
		patch::SetRaw(0x6A2EA5, (void *)"\x66\x39\xC0\x90", 4);
		patch::SetRaw(0x6BDE63, (void *)"\x66\x39\xC0\x90\x90\x90\x90", 7);
	}

	Events::initGameEvent += []()
	{
		LightsGlobal::Get().gbGlobalIndicatorLights = gConfig.ReadBoolean("FEATURES", "StandardLights_GlobalIndicatorLights", false);
		LightsGlobal::Get().gfGlobalCoronaSize = gConfig.ReadFloat("VISUAL", "LightCoronaSize", 0.3f);
		LightsGlobal::Get().gGlobalShadowIntensity = gConfig.ReadInteger("VISUAL", "LightShadowIntensity", 220);
		LightsGlobal::Get().gGlobalCoronaIntensity = gConfig.ReadInteger("VISUAL", "LightCoronaIntensity", 250);
	};

    ModelInfoMgr::RegisterMaterial([](CVehicle *pVeh, RpMaterial *pMat) {
        return LightManager::GetMatType(pMat); 
    });

	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view& nodeName) {
        LightManager::RegisterDummy(pVeh, pFrame, std::string(nodeName));
    });

	Events::processScriptsEvent += []()
	{
		CVehicle *pVeh = FindPlayerVehicle(-1, false);
        if (pVeh) {
            LightManager::Process(pVeh);
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