#include "pch.h"
#include "spotlights.h"
#include <CCamera.h>
#include <CCoronas.h>

SpotLightFeature SpotLight;
void SpotLightFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {    
	if (!bHooksInjected) {
		Events::vehicleRenderEvent += [this](CVehicle *pVeh) {
			OnVehicleRender(pVeh);
		};

		Events::drawingEvent += [this]() {
			OnHudRender();
		};
		pSpotlightTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("/textures/spotlight256.png")));
		bHooksInjected = true;
	}
    VehData &data = vehData.Get(pVeh);
    data.pFrame = pFrame;
}

void SpotLightFeature::Process(RwFrame* pFrame, CVehicle* pVeh) {    
    VehData &data = vehData.Get(pVeh);
    
	if (!data.bInit) {
		Initialize(pFrame, pVeh);
		data.bInit = true;
	}
}

void SpotLightFeature::OnHudRender() {
	CVehicle *pVeh = FindPlayerVehicle(-1, false);

	if (!pVeh) {
		return;
	}

	VehData &data = vehData.Get(pVeh);

	static size_t prev = 0;
	if (KeyPressed(VK_B)) {
		size_t now = CTimer::m_snTimeInMilliseconds;
		if (now - prev > 500.0f) {

			data.bEnabled = !data.bEnabled;
			prev = now;
		}
	}

	if (!plugin::KeyPressed(0x02))
		return;

	CCamera* camera = (CCamera*)0xB6F028;

	float heading = camera->GetHeading();

	CMatrix matrix;

	matrix = camera->m_mCameraMatrix;

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

	RwFrameRotate(data.pFrame, (RwV3d*)0x008D2E18, vehicleHeading, rwCOMBINEPRECONCAT);

	//RwFrameRotate(Frame, (RwV3d*)0x008D2E18, heading, rwCOMBINEREPLACE);
};

void SpotLightFeature::OnVehicleRender(CVehicle *pVeh) {
	VehData &data = vehData.Get(pVeh);
	if (!data.bEnabled || data.pFrame == nullptr)
		return;

	CCamera* camera = (CCamera*)0xB6F028;

	float cameraHeading = camera->GetHeading();

	float vehicleHeading = pVeh->GetHeading();

	CMatrix matrix = CMatrix(new RwMatrix(data.pFrame->modelling), true);

	matrix.pos += ((RwFrame*)data.pFrame->object.parent)->modelling.pos;

	float vehicleAngle = vehicleHeading * 180.0f / 3.14f;

	float cameraAngle = cameraHeading * 180.0f / 3.14f;

	float differenceAngle = ((cameraAngle > vehicleAngle) ? (cameraAngle - vehicleAngle) : (vehicleAngle - cameraAngle));

	if (differenceAngle > 90.0f && differenceAngle < 270.0f) {
		CVector position = CVector(matrix.pos);

		CCoronas::RegisterCorona(reinterpret_cast<unsigned int>(pVeh) + 49, pVeh, 255, 255, 255, 128, position,
			0.3f, 300.0f, eCoronaType::CORONATYPE_SHINYSTAR, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 90.0f, false, 1.0f, 0, 50.0f, false, true);
	}

	matrix.RotateZ(vehicleHeading);

	/*CShadows::StoreShadowToBeRendered(SHADOW_DEFAULT, VehicleTextures::Spotlight, new CVector(Vehicle->GetPosition()),
		0.0f, 2.0f, 0.0f, 2.0f,
		1,
		255, 255, 255,
		160.0f, false, 1.0f, new CRealTimeShadow(), true);*/

	/*CShadows::StoreCarLightShadow(Vehicle, reinterpret_cast<unsigned int>(Vehicle) + 25, VehicleTextures::Spotlight, &Vehicle->GetPosition(),
		2.0f, 0.0f, 0.0f, 2.0f,
		128, 128, 128, 0.0f);*/

	/*CVector Pos = CModelInfo::ms_modelInfoPtrs[Vehicle->m_nModelIndex]->m_pColModel->m_boundBox.m_vecMin;

	CVector center = Vehicle->TransformFromObjectSpace(CVector(0.0f, 0.0f, 0.0f));

	CVector up = Vehicle->TransformFromObjectSpace(CVector(0.0f, -Pos.y - 0.5f, 0.0f)) - center;
	CVector right = Vehicle->TransformFromObjectSpace(CVector(Pos.x + 0.2f, 0.0f, 0.0f)) - center;
	CShadows::StoreShadowToBeRendered(2, VehicleTextures::Spotlight, &center, up.x, up.y, right.x, right.y, 255, 128, 128, 128, 2.0f, false, 1.0f, 0, true);*/

	pVeh->DoHeadLightReflectionSingle(matrix, 1);
};