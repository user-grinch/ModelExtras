#include "pch.h"
#include "spotlights.h"
#include <CCamera.h>
#include <CCoronas.h>
#include "defines.h"

void SpotLights::Process(void *ptr, RwFrame *pFrame, eModelEntityType type)
{
	CVehicle *pVeh = static_cast<CVehicle *>(ptr);
	VehData &data = vehData.Get(pVeh);

	if (!data.bInit)
	{
		VehData &data = vehData.Get(pVeh);
		data.pFrame = pFrame;
		data.bInit = true;
	}
}

void SpotLights::Initialize()
{
	Events::vehicleRenderEvent += [](CVehicle *pVeh)
	{
		OnVehicleRender(pVeh);
	};

	Events::drawingEvent += []()
	{
		OnHudRender();
	};
	pSpotlightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("/textures/spotlight.png")));
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
	if (KeyPressed(VK_B))
	{
		size_t now = CTimer::m_snTimeInMilliseconds;
		if (now - prev > 500.0f)
		{
			data.bEnabled = !data.bEnabled;
			prev = now;
		}
	}

	if (!plugin::KeyPressed(0x02))
		return;

	float heading = TheCamera.GetHeading();

	CMatrix matrix;

	matrix = TheCamera.m_mCameraMatrix;

	data.pFrame->modelling.at.x = matrix.at.x;
	data.pFrame->modelling.at.y = matrix.at.y;
	data.pFrame->modelling.at.z = matrix.at.z;

	data.pFrame->modelling.right.x = matrix.right.x;
	data.pFrame->modelling.right.y = matrix.right.y;
	data.pFrame->modelling.right.z = matrix.right.z;

	data.pFrame->modelling.up.x = matrix.up.x;
	data.pFrame->modelling.up.y = matrix.up.y;
	data.pFrame->modelling.up.z = matrix.up.z;

	float vehicleHeading = pVeh->GetHeading() * 180.0f / 3.14f;
	RwFrameRotate(data.pFrame, (RwV3d *)0x008D2E18, vehicleHeading, rwCOMBINEPRECONCAT);
};

void SpotLights::OnVehicleRender(CVehicle *pVeh)
{
	VehData &data = vehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
		return;

	float cameraHeading = TheCamera.GetHeading();

	float vehicleHeading = pVeh->GetHeading();

	CMatrix matrix = CMatrix(new RwMatrix(data.pFrame->modelling), true);

	matrix.pos += ((RwFrame *)data.pFrame->object.parent)->modelling.pos;

	matrix.RotateZ(vehicleHeading);

	pVeh->DoHeadLightReflectionSingle(matrix, 1);
};