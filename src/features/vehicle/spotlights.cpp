#include "pch.h"
#include "spotlights.h"
#include <CCamera.h>
#include <CCoronas.h>
#include "defines.h"
#include "texmgr.h"
#include <CWorld.h>
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include "core/materials.h"

#define VK_RMB 0x02

void SpotLights::Initialize()
{

	VehicleMaterials::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, std::string name, bool parent)
									{
        VehData& data = vehData.Get(pVeh);
        if (name == "spotlight_dummy") data.pFrame = pFrame; });

	Events::vehicleRenderEvent += [](CVehicle *pVeh)
	{
		OnVehicleRender(pVeh);
	};

	Events::drawingEvent += []()
	{
		OnHudRender();
	};
	Events::initGameEvent += []()
	{
		pSpotlightTex = TextureMgr::Get("spotlight", 30);
	};
}

bool SpotLights::IsEnabled(CVehicle *pVeh)
{
	return vehData.Get(pVeh).bEnabled;
}

void SpotLights::OnHudRender()
{
	CVehicle *pVeh = FindPlayerVehicle(-1, false);

	if (!pVeh)
	{
		return;
	}

	VehData &data = vehData.Get(pVeh);

	static size_t prev = 0;
	static uint32_t key = gConfig.ReadInteger("KEYS", "SpotLightKey", VK_B);
	if (KeyPressed(key))
	{
		size_t now = CTimer::m_snTimeInMilliseconds;
		if (now - prev > 500.0f)
		{
			data.bEnabled = !data.bEnabled;
			prev = now;
		}
	}

	if (!plugin::KeyPressed(VK_RMB) || !data.pFrame)
	{
		return;
	}

	data.pFrame->modelling = *(RwMatrix *)&TheCamera.m_mCameraMatrix;
	float heading = pVeh->GetHeading() * 180.0f / 3.14f;
	Util::SetFrameRotationZ(data.pFrame, heading);
};

void SpotLights::OnVehicleRender(CVehicle *pVeh)
{
	VehData &data = vehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
		return;

	float vehicleHeading = pVeh->GetHeading();
	CMatrix matrix = *(CMatrix *)&data.pFrame->modelling;
	matrix.pos += ((RwFrame *)data.pFrame->object.parent)->modelling.pos;
	matrix.RotateZ(vehicleHeading);

	pVeh->DoHeadLightReflectionSingle(matrix, 1);
	RwV3d offset{0, 0, 0}, target, src;
	CVector vehPos = pVeh->GetPosition();
	RwV3dTransformPoint(&src, &offset, &data.pFrame->modelling);
	RwV3dTransformPoint(&target, &offset, (RwMatrix *)&matrix);

	// target.x += vehPos.x;
	// target.y += vehPos.y;
	// target.z += vehPos.z;
	// src.x += vehPos.x;
	// src.y += vehPos.y;
	// src.z += vehPos.z;

	bool flag;
	CEntity *pEnt;
	target.z = CWorld::FindGroundZFor3DCoord(target.x, target.y, target.z + 20, &flag, &pEnt);
	static int searchLight = NULL;

	if (searchLight != NULL)
	{
		plugin::Command<plugin::Commands::DELETE_SEARCHLIGHT>(searchLight);
		searchLight = NULL;
		// plugin::Command<plugin::Commands::CREATE_SEARCHLIGHT>(-2025.600342, 176.275925, 48.843737, -2025.600342, 176.275925, 38.843737, 10.0, 1.0, &searchLight);
	}
	plugin::Command<plugin::Commands::CREATE_SEARCHLIGHT>(target.x, target.y, target.z, src.x, src.y, src.z, 1.0, 0.05, &searchLight);
};