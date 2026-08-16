#include "pch.h"
#include "spotlights.h"
#include <CCamera.h>
#include "defines.h"
#include "utils/texmgr.h"
#include <CWorld.h>
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include "utils/modelinfomgr.h"

using namespace plugin;

#define VK_RMB 0x02

void SpotLights::Init()
{
	ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
							   {
        SpotlightData& data = m_VehData.Get(pVeh);
		std::string name = GetFrameNodeName(pFrame);
        if (name == "spotlight_dummy") data.pFrame = pFrame; });

	Events::vehicleRenderEvent += [](CVehicle *pVeh)
	{
		if (!pVeh || !pVeh->GetIsOnScreen())
		{
			return;
		}
		OnVehicleRender(pVeh);
	};

	Events::drawingEvent += []()
	{
		OnHudRender();
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

	if (!KeyPressed(VK_RMB) || !data.pFrame)
	{
		return;
	}

	data.pFrame->modelling = *(RwMatrix *)&TheCamera.m_mCameraMatrix;
	float heading = pVeh->GetHeading() * 180.0f / 3.14f;
	FrameUtil::SetRotationZ(data.pFrame, heading);
};

void SpotLights::OnVehicleRender(CVehicle *pVeh)
{
	SpotlightData &data = m_VehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
		return;

	float vehicleHeading = pVeh->GetHeading();
	CMatrix matrix = *(CMatrix *)&data.pFrame->modelling;
	matrix.pos += ((RwFrame *)data.pFrame->object.parent)->modelling.pos;
	matrix.RotateZ(vehicleHeading);

	pVeh->DoHeadLightReflectionSingle(matrix, 1);
	RwV3d offset{0, 0, 0}, target, src;
	RwV3dTransformPoint(&src, &offset, &data.pFrame->modelling);
	RwV3dTransformPoint(&target, &offset, (RwMatrix *)&matrix);

	bool flag;
	CEntity *pEnt;
	target.z = CWorld::FindGroundZFor3DCoord(target.x, target.y, target.z + 20, &flag, &pEnt);
	static int searchLight = 0;

	if (searchLight != 0)
	{
		Command<Commands::DELETE_SEARCHLIGHT>(searchLight);
		searchLight = 0;
	}
	Command<Commands::CREATE_SEARCHLIGHT>(target.x, target.y, target.z, src.x, src.y, src.z, 1.0, 0.05, &searchLight);
};