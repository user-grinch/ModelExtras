#include "enums/materialtype.h"
#include "pch.h"
#include "lights.h"
#include "manager.h"
#include "utils/meevents.h"
#include "utils/datamgr.h"
#include "ModelExtrasAPI.h"
#include "utils/samp.h"
#include <CCamera.h>
#include <eModelID.h>

float gfGlobalCoronaSize = 0.3f;
int gGlobalCoronaIntensity = 80;
int gGlobalShadowIntensity = 80;
bool gbLightPointLights = true;
bool gbSirenPointLights = false;

static void __fastcall Hooked_DoHeadLightBeam(CVehicle *pVeh, void *edx, int dummyId, CMatrix &matrix, bool arg2)
{
	if (!pVeh || !Lights::m_bEnabled) return;
	if (!gConfig.ReadBoolean("LIGHTS", "HeadLightBeams", gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true))) return;

	auto *mi = reinterpret_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
	if (!mi || !mi->m_pVehicleStruct) return;

	int dummyIndex = 2 * dummyId;
	if (dummyIndex < 0 || dummyIndex >= 15) return;

	CVector pointModelSpace = mi->m_pVehicleStruct->m_avDummyPos[dummyIndex];
	if (dummyId == 1 && pointModelSpace.Magnitude() < 0.0001f) return;

	CVector point = matrix * pointModelSpace;
	if (!arg2) {
		point -= matrix.GetRight() * (2.0f * pointModelSpace.x);
	}

	CVector pointToCamDir = TheCamera.GetPosition() - point;
	pointToCamDir.Normalize();
	float dot = pointToCamDir.x * matrix.GetForward().x + pointToCamDir.y * matrix.GetForward().y + pointToCamDir.z * matrix.GetForward().z;
	unsigned char alpha = static_cast<unsigned char>((1.0f - std::fabs(dot)) * 32.0f);
	if (alpha == 0) return;

	float angleMult = (pVeh->m_nModelIndex == MODEL_FORKLIFT) ? 0.5f : 0.15f;
	CVector lightNormal = matrix.GetForward() - matrix.GetUp() * angleMult;
	lightNormal.Normalize();
	CVector lightRight;
	RwV3dCrossProduct(&lightRight, &lightNormal, &pointToCamDir);
	lightRight.Normalize();
	CVector lightPos = point - matrix.GetForward() * 0.1f;

	CVector posn[5] = {
		lightPos - lightRight * 0.05f,
		lightPos + lightRight * 0.05f,
		lightPos + lightNormal * 3.0f - lightRight * 0.5f,
		lightPos + lightNormal * 3.0f + lightRight * 0.5f,
		lightPos + lightNormal * 0.2f
	};
	unsigned char alphas[5] = { alpha, alpha, 0, 0, alpha };

	eMaterialType matType = arg2 ? eMaterialType::HeadLightLeft : eMaterialType::HeadLightRight;
	CRGBA lightCol = LightManager::GetMaterialColor(pVeh, matType).on;

	RwIm3DVertex vertices[5];
	for (int i = 0; i < 5; ++i) {
		unsigned char r = lightCol.r;
		unsigned char g = lightCol.g;
		unsigned char b = lightCol.b;
		unsigned char a = static_cast<unsigned char>((alphas[i] * lightCol.a) / 255);
		RwIm3DVertexSetRGBA(&vertices[i], r, g, b, a);
		RwIm3DVertexSetPos(&vertices[i], posn[i].x, posn[i].y, posn[i].z);
	}

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void *>(FALSE));
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void *>(TRUE));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void *>(TRUE));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void *>(rwBLENDSRCALPHA));
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void *>(rwBLENDONE));
	RwRenderStateSet(rwRENDERSTATESHADEMODE, reinterpret_cast<void *>(rwSHADEMODEGOURAUD));
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATECULLMODE, reinterpret_cast<void *>(rwCULLMODECULLNONE));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, reinterpret_cast<void *>(rwALPHATESTFUNCTIONGREATER));
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, reinterpret_cast<void *>(FALSE));

	if (RwIm3DTransform(vertices, 5, nullptr, rwIM3D_VERTEXRGBA | rwIM3D_VERTEXXYZ)) {
		static RxVertexIndex indices[12] = { 0, 1, 4, 1, 3, 4, 2, 3, 4, 0, 2, 4 };
		RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, indices, 12);
		RwIm3DEnd();
	}

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void *>(TRUE));
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void *>(TRUE));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, reinterpret_cast<void *>(rwBLENDSRCALPHA));
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, reinterpret_cast<void *>(rwBLENDINVSRCALPHA));
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void *>(FALSE));
	RwRenderStateSet(rwRENDERSTATECULLMODE, reinterpret_cast<void *>(rwCULLMODECULLBACK));
}

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

	// CVehicle::DoVehicleLights (native headlight coronas)
	patch::SetUChar(0x6E2193, 0);
	patch::SetUChar(0x6E228B, 0);
	patch::SetUChar(0x6E2532, 0);
	patch::SetUChar(0x6E2627, 0);

	SAMP::PatchVehicleLights();

	// Dynamic CVehicle::DoHeadLightBeam
	patch::ReplaceFunction(0x6E0E20, (void *)Hooked_DoHeadLightBeam);

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
        if (!m_bEnabled) return MatStateColor{DEFAULT_MAT_COL, DEFAULT_MAT_COL};
        return LightManager::GetMaterialColor(pVeh, type);
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
