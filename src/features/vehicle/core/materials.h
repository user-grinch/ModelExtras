#pragma once
#include <string>
#include <map>
#include "game_sa/CVehicle.h"
#include "game_sa/CPools.h"
#include "game_sa/CModelInfo.h"
#include "game_sa/NodeName.h"
#include "RenderWare.h"
#include "enums/dummypos.h"

class VehicleMaterial
{
public:
	RpMaterial *Material;
	RwTexture *Texture;
	RwTexture *TextureActive;
	CRGBA Color;
	eDummyPos Pos;

	VehicleMaterial(RpMaterial *material, eDummyPos pos = (eDummyPos)0);
};

class VehicleMaterials
{
public:
	static void Register(std::function<RpMaterial *(CVehicle *, RpMaterial *, CRGBA)> function);
	static void RegisterRender(std::function<void(CVehicle *)> render);
	static void RegisterDummy(std::function<void(CVehicle *, RwFrame *, std::string, bool)> function);
	static void OnModelSet(CVehicle *vehicle, int model);
	static void OnRender(CVehicle *vehicke);
	static void StoreMaterial(std::pair<unsigned int *, unsigned int> pair);
	static void RestoreMaterials();

private:
	static inline std::vector<std::function<RpMaterial *(CVehicle *, RpMaterial *, CRGBA)>> functions;
	static inline std::vector<std::function<void(CVehicle *)>> renders;
	static inline std::vector<std::function<void(CVehicle *, RwFrame *, std::string, bool)>> dummy;
	/*
	 *	Note: Material data need to be model based
	 *		  Dummy data should be entity based
	 *		  Don't change it
	 */
	static inline std::map<int, std::map<RpMaterial *, bool>> materials;
	static inline CVehicle *currentVehicle;
	static inline std::list<std::pair<unsigned int *, unsigned int>> storedMaterials;

public:
	static void FindDummies(CVehicle *vehicle, RwFrame *frame, bool parent = false, bool print = false);
};