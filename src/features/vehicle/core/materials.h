#pragma once
#include <string>
#include <map>
#include "game_sa/CVehicle.h"
#include "game_sa/CPools.h"
#include "game_sa/CModelInfo.h"
#include "game_sa/NodeName.h"
#include "RenderWare.h"
#include "enums/dummypos.h"
#include "enums/lighttype.h"

using DummyCallback_t = std::function<void(CVehicle *, RwFrame *)>;
using RenderCallback_t = std::function<void(CVehicle *)>;
using MaterialCallback_t = std::function<eLightType(CVehicle *, RpMaterial*)>;

class ModelInfoMgr
{
private:
	static inline std::vector<DummyCallback_t> dummy;
	static inline std::vector<MaterialCallback_t> materials;
	static inline std::vector<RenderCallback_t> renders;

	static inline std::map<CVehicle *, std::array<bool, TotalLight>> m_LightStatus;

	static void FindDummies(CVehicle *vehicle, RwFrame *frame);
	static void OnRender(CVehicle *pVeh);

	static RpMaterial *SetEditableMaterialsCB(RpMaterial *material, void *data);
	static void __fastcall SetupRender(CVehicle *ptr);

public:
	static void EnableLight(CVehicle *pVeh, eLightType type);
	static void Initialize();
	static void RegisterDummy(DummyCallback_t function);
	static void RegisterMaterial(MaterialCallback_t material);
	static void RegisterRender(RenderCallback_t render);

	static inline std::map<CVehicle *, std::array<bool, 256>> m_SirenStatus;
};
