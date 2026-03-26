#pragma once
#include <string>
#include <map>
#include "game_sa/CVehicle.h"
#include "game_sa/CPools.h"
#include "game_sa/CModelInfo.h"
#include "RenderWare.h"
#include "enums/dummypos.h"
#include "enums/materialtype.h"

struct MatStateColor
{
	CRGBA on, off;
};

constexpr uint32_t MAX_LIGHTS = 256;
const CRGBA DEFAULT_MAT_COL = CRGBA(255, 255, 255, 255);
using DummyCallback_t = std::function<void(CVehicle *, RwFrame *)>;
using RenderCallback_t = std::function<void(CVehicle *)>;
using MaterialCallback_t = std::function<eMaterialType(CVehicle *, RpMaterial *)>;
using MaterialColProviderCallback_t = std::function<MatStateColor(CVehicle *, RpMaterial *, eMaterialType)>;

struct VehModelData
{
	bool *m_MatStatus;
	bool *m_MatAvail;
	bool *m_SirenStatus;
	bool *m_StrobeStatus;
	uint32_t nFrameCount = 0;

	VehModelData(CVehicle *pVeh)
	{
		m_MatStatus = new bool[eMaterialType::TotalMaterial]();
		m_MatAvail = new bool[eMaterialType::TotalMaterial]();
		m_SirenStatus = new bool[MAX_LIGHTS]();
		m_StrobeStatus = new bool[MAX_LIGHTS]();
	}

	~VehModelData()
	{
		delete[] m_MatStatus;
		delete[] m_MatAvail;
		delete[] m_SirenStatus;
		delete[] m_StrobeStatus;
	}

	VehModelData(const VehModelData &) = delete;
	VehModelData &operator=(const VehModelData &) = delete;

	VehModelData(VehModelData &&other) noexcept
	{
		m_MatStatus = other.m_MatStatus;
		m_MatAvail = other.m_MatAvail;
		m_SirenStatus = other.m_SirenStatus;
		m_StrobeStatus = other.m_StrobeStatus;
		other.m_MatStatus = other.m_MatAvail = other.m_SirenStatus = other.m_StrobeStatus = nullptr;
	}

	VehModelData &operator=(VehModelData &&other) noexcept
	{
		if (this != &other)
		{
			delete[] m_MatStatus;
			delete[] m_MatAvail;
			delete[] m_SirenStatus;
			delete[] m_StrobeStatus;

			m_MatStatus = other.m_MatStatus;
			m_MatAvail = other.m_MatAvail;
			m_SirenStatus = other.m_SirenStatus;
			m_StrobeStatus = other.m_StrobeStatus;
			other.m_MatStatus = other.m_MatAvail = other.m_SirenStatus = other.m_StrobeStatus = nullptr;
		}
		return *this;
	}
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
	static MatStateColor FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat, eMaterialType type);
	static eMaterialType FetchMaterialType(CVehicle *pVeh, RpMaterial *pMat);

	static RpMaterial *SetEditableMaterialsCB(RpMaterial *material, void *data);
	static void __fastcall SetupRender(CVehicle *ptr);

public:
	static void EnableMaterial(CVehicle *pVeh, eMaterialType type);
	static void EnableSirenMaterial(CVehicle *pVeh, int idx);
	static void EnableStrobeMaterial(CVehicle *pVeh, int idx);
	static bool IsMaterialAvailable(CVehicle *pVeh, eMaterialType type);

	static void Initialize();
	static void RegisterDummy(DummyCallback_t function);
	static void RegisterMaterial(MaterialCallback_t material);
	static void RegisterMaterialColProvider(MaterialColProviderCallback_t material);
	static void RegisterRender(RenderCallback_t render);
	static void Reload(CVehicle *pVeh);
};
