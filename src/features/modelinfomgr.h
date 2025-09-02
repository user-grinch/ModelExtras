#pragma once
#include <string>
#include <map>
#include "game_sa/CVehicle.h"
#include "game_sa/CPools.h"
#include "game_sa/CModelInfo.h"
#include "RenderWare.h"
#include "enums/dummypos.h"
#include "enums/lighttype.h"

struct MatStateColor {
	CRGBA on, off;
};

constexpr uint32_t MAX_LIGHTS = 256;
const CRGBA DEFAULT_MAT_COL = CRGBA(255, 255, 255, 255);
using DummyCallback_t = std::function<void(CVehicle *, RwFrame *)>;
using RenderCallback_t = std::function<void(CVehicle *)>;
using MaterialCallback_t = std::function<eLightType(CVehicle *, RpMaterial*)>;
using MaterialColProviderCallback_t = std::function<MatStateColor(CVehicle *, RpMaterial*, eLightType)>;

struct VehModelData {
	std::array<bool, TotalLight> m_LightStatus;
	std::array<bool, TotalLight> m_MatAvail;
	std::array<bool, MAX_LIGHTS> m_SirenStatus, m_StrobeStatus;

	VehModelData(CVehicle *pVeh) {
		std::fill(std::begin(m_LightStatus), std::end(m_LightStatus), false);
		std::fill(std::begin(m_MatAvail), std::end(m_MatAvail), false);
		std::fill(std::begin(m_SirenStatus), std::end(m_SirenStatus), false);
		std::fill(std::begin(m_StrobeStatus), std::end(m_StrobeStatus), false);
	}
	~VehModelData() {}
};

class ModelInfoMgr
{
private:
	static inline std::vector<DummyCallback_t> dummy;
	static inline std::vector<MaterialCallback_t> materials;
	static inline std::vector<MaterialColProviderCallback_t> matColProviders;
	static inline std::vector<RenderCallback_t> renders;

	static inline VehicleExtendedData<VehModelData> m_VehData;

	static void FindDummies(CVehicle *vehicle, RwFrame *frame);
	static void OnRender(CVehicle *pVeh);
	static MatStateColor FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat, eLightType type);
	static eLightType FetchMaterialType(CVehicle *pVeh, RpMaterial *pMat);

	static RpMaterial *SetEditableMaterialsCB(RpMaterial *material, void *data);
	static void __fastcall SetupRender(CVehicle *ptr);

public:
	static void EnableLightMaterial(CVehicle *pVeh, eLightType type);
	static void EnableSirenMaterial(CVehicle *pVeh, int idx);
	static void EnableStrobeMaterial(CVehicle *pVeh, int idx);
	static bool IsMaterialAvailable(CVehicle *pVeh, eLightType type);

	static void Initialize();
	static void RegisterDummy(DummyCallback_t function);
	static void RegisterMaterial(MaterialCallback_t material);
	static void RegisterMaterialColProvider(MaterialColProviderCallback_t material);
	static void RegisterRender(RenderCallback_t render);
	static void Reload(CVehicle *pVeh);
};
