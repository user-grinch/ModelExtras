#include "pch.h"
#include "spotlights.h"
#include <CCamera.h>
#include <CCoronas.h>
#include <CShadows.h>
#include <CPointLights.h>
#include <CPools.h>
#include <CWorld.h>
#include "defines.h"
#include "utils/texmgr.h"
#include "utils/modelinfomgr.h"
#include "utils/audiomgr.h"
#include "utils/util.h"
#include "utils/meevents.h"

using namespace plugin;

#define VK_RMB 0x02
extern int gGlobalShadowIntensity;

static inline CVector2D GetPerpRight(const CVector2D &vec)
{
	return {vec.y, -vec.x};
}

void SpotLights::Init()
{
	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
	{
		SpotlightData &data = m_VehData.Get(pVeh);
		if (nodeName == "spotlight_dummy")
		{
			data.pFrame = pFrame;
			data.origPos = pFrame->modelling.pos;
			data.bHasOrigPos = true;
		}
	});

	Events::vehicleRenderEvent += [](CVehicle *pVeh)
	{
		if (!pVeh || !pVeh->GetIsOnScreen())
		{
			return;
		}
		OnVehicleRender(pVeh);
	};

	MEEvents::vehPreRenderEvent.before += [](CVehicle *pVeh)
	{
		ProcessPointLights(pVeh);
	};

	Events::processScriptsEvent += []()
	{
		for (CVehicle *pVeh : CPools::ms_pVehiclePool)
		{
			if (pVeh && pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
			{
				ProcessPointLights(pVeh);
			}
		}
	};

	Events::drawingEvent += []()
	{
		OnHudRender();
	};

	Events::initGameEvent += []()
	{
		pSpotlightTex = TextureMgr::Get("spotlight", gGlobalShadowIntensity);
		if (!pSpotlightTex)
		{
			pSpotlightTex = TextureMgr::Get("round", gGlobalShadowIntensity);
		}
	};
}

bool SpotLights::IsEnabled(CVehicle *pVeh)
{
	return m_VehData.Get(pVeh).bEnabled;
}

void SpotLights::OnHudRender()
{
	CVehicle *pVeh = FindPlayerVehicle(-1, false);
	if (!pVeh)
	{
		return;
	}

	SpotlightData &data = m_VehData.Get(pVeh);
	if (!data.pFrame)
	{
		return;
	}

	static size_t prev = 0;
	static uint32_t key = gConfig.ReadInteger("KEYS", "SpotLightKey", VK_B);
	if (Util::IsKeyPressed(key))
	{
		size_t now = CTimer::m_snTimeInMilliseconds;
		if (now - prev > 500.0f)
		{
			data.bEnabled = !data.bEnabled;
			prev = now;
			AudioMgr::PlaySwitchSound(pVeh);
		}
	}

	if (!Util::IsKeyPressed(VK_RMB))
	{
		return;
	}

	if (!data.bHasOrigPos)
	{
		data.origPos = data.pFrame->modelling.pos;
		data.bHasOrigPos = true;
	}

	// Aiming: Align spotlight frame orientation with camera look direction (AVS method)
	data.pFrame->modelling.right = *(RwV3d *)&TheCamera.m_mCameraMatrix.right;
	data.pFrame->modelling.up = *(RwV3d *)&TheCamera.m_mCameraMatrix.up;
	data.pFrame->modelling.at = *(RwV3d *)&TheCamera.m_mCameraMatrix.at;
	data.pFrame->modelling.pos = data.origPos;

	float vehicleHeadingDeg = pVeh->GetHeading() * 180.0f / 3.14159265f;
	static RwV3d axisZ = {0.0f, 0.0f, 1.0f};
	RwFrameRotate(data.pFrame, &axisZ, vehicleHeadingDeg, rwCOMBINEPRECONCAT);
	data.pFrame->modelling.pos = data.origPos;

	RwFrameUpdateObjects(data.pFrame);
}

void SpotLights::OnVehicleRender(CVehicle *pVeh)
{
	SpotlightData &data = m_VehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
		return;

	data.nLastFrame = CTimer::m_FrameCounter;
	RwFrameUpdateObjects(data.pFrame);
	CMatrix &frameLtm = *(CMatrix *)&data.pFrame->ltm;

	CVector lightPos = frameLtm.pos;
	CVector lightDir = frameLtm.up; // Forward vector of spotlight
	lightDir.Normalize();

	// 1. Corona at the spotlight lamp position (strictly visible when looking from the front of the lamp)
	CVector toCam = TheCamera.GetPosition() - lightPos;
	toCam.Normalize();
	float dot = CVector::Dot(lightDir, toCam);
	if (dot > 0.25f)
	{
		float alphaMul = std::clamp((dot - 0.25f) / 0.5f, 0.0f, 1.0f);
		CRGBA col = {255, 255, 255, static_cast<unsigned char>(220 * alphaMul)};
		CCoronas::RegisterCorona(
			reinterpret_cast<unsigned int>(pVeh) + 49,
			pVeh,
			col.r, col.g, col.b, col.a,
			data.pFrame->modelling.pos,
			0.35f,
			250.0f,
			CORONATYPE_SHINYSTAR,
			FLARETYPE_NONE,
			false,
			false,
			0,
			0.0f,
			false,
			0.45f,
			0,
			30.0f,
			false,
			true
		);
	}

	// 2. Enable spotlight material glow on the vehicle
	ModelInfoMgr::EnableMaterial(pVeh, eMaterialType::SpotLight);
}

void SpotLights::ProcessPointLights(CVehicle *pVeh)
{
	extern bool gbLightPointLights;
	if (!gbLightPointLights || !pVeh || pVeh->m_fHealth <= 0.0f)
	{
		return;
	}

	SpotlightData &data = m_VehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
	{
		return;
	}

	if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) > 120.0f)
	{
		return;
	}

	RwFrameUpdateObjects(data.pFrame);
	CMatrix &frameLtm = *(CMatrix *)&data.pFrame->ltm;

	CVector lightPos = frameLtm.pos;
	CVector lightDir = frameLtm.up;
	lightDir.Normalize();

	// 1. 3D Point Light: Placed forward along cone (3.5m) with 18m reach
	CVector plightPos = lightPos + lightDir * 3.5f;
	CPointLights::AddLight(
		PLTYPE_SPOTLIGHT,
		plightPos,
		lightDir,
		18.0f,
		1.2f, 1.2f, 1.2f,
		0,
		false,
		nullptr
	);

	// 2. Ground Shadow: Disabled with ProperShaders (unless PointLights is disabled)
	extern bool gbProperShadersDetected;
	extern bool gbLightPointLights;
	if (!gbProperShadersDetected || !gbLightPointLights)
	{
		RwTexture *pTex = pSpotlightTex ? pSpotlightTex : TextureMgr::Get("spotlight", gGlobalShadowIntensity);
		if (!pTex)
		{
			pTex = TextureMgr::Get("round", gGlobalShadowIntensity);
		}

		if (pTex)
		{
			float castDist = 12.5f;
			CVector shadowCenter = lightPos + lightDir * castDist;
			bool groundFound = false;
			CEntity *pGroundEntity = nullptr;
			float groundZ = CWorld::FindGroundZFor3DCoord(shadowCenter.x, shadowCenter.y, shadowCenter.z + 10.0f, &groundFound, &pGroundEntity);
			if (groundFound)
			{
				shadowCenter.z = groundZ + 0.05f;
			}

			CVector2D front2D(lightDir.x, lightDir.y);
			front2D.Normalize();
			float shdwLength = 3.2f;
			float shdwWidth = 3.8f;
			CVector2D shdwFront = front2D * (shdwLength * 2.0f);
			CVector2D shdwSide = GetPerpRight(front2D * shdwWidth);

			float distToCam = CVector::Distance(shadowCenter, TheCamera.GetPosition());
			if (distToCam < 130.0f)
			{
				float alphaMul = 1.0f;
				if (distToCam > 80.0f)
				{
					alphaMul = (130.0f - distToCam) / 50.0f;
				}
				CShadows::StoreShadowToBeRendered(
					2,
					pTex,
					&shadowCenter,
					shdwFront.x, shdwFront.y,
					shdwSide.x, shdwSide.y,
					static_cast<short>(230 * alphaMul),
					255, 255, 255,
					8.0f,
					false,
					1.0f,
					0,
					true);
			}
		}
	}
}