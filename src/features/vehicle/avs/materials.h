#pragma once

#include <string>
#include <map>
#include <vector>
#include <list>
#include "game_sa/CVehicle.h"
#include "game_sa/CPools.h"
#include "game_sa/CModelInfo.h"
#include "game_sa/NodeName.h"
#include "game_sa/RenderWare.h"

typedef RpMaterial* (*VehicleMaterialFunction)(CVehicle* vehicle, RpMaterial* material);
typedef void (*VehicleMaterialRender)(CVehicle* vehicle, int index);
typedef void (*VehicleDummyFunction)(CVehicle* vehicle, RwFrame* frame, std::string name, bool parent);

class VehicleMaterial {
public:
    RpMaterial* Material;
    RwTexture* Texture;
    RwTexture* TextureActive;
    RwRGBA Color;

    VehicleMaterial(RpMaterial* material);
};

class VehicleMaterials {
private:
    static inline std::vector<VehicleMaterialFunction> functions;
    static inline std::vector<VehicleMaterialRender> renders;
    static inline std::vector<VehicleDummyFunction> dummy;
    static inline std::map<int, std::map<RpMaterial*, bool>> materials;
    static inline std::map<int, std::map<RwFrame*, bool>> frames;
    static inline std::map<int, bool> dummies;
    static inline CVehicle* currentVehicle;
    static inline std::list<std::pair<unsigned int*, unsigned int>> storedMaterials;

public:
    static void Register(VehicleMaterialFunction function);
    static void RegisterRender(VehicleMaterialRender render);
    static void RegisterDummy(VehicleDummyFunction function);
    static void OnModelSet(CVehicle* vehicle, int model);
    static void OnRender(CVehicle* vehicle);
    static void StoreMaterial(std::pair<unsigned int*, unsigned int> pair);
    static void RestoreMaterials();

private:
    static void findDummies(CVehicle* vehicle, RwFrame* frame, bool parent = false);
};