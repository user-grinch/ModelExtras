#pragma once
#include <map>
#include <string>
#include <vector>

#include <plugin.h>
#include <CCoronas.h>
#include "../../interface/ifeature.hpp"
#include "internals/dummy.h"
#include "internals/materials.h"

enum class VehicleSirenType { 
	Directional, 
	NonDirectional, 
	Inversed, 
	Rotator
};

enum class VehicleSirenStates { 
	Off, 
	On, 
	Mute 
};

struct VehicleSirenShadow {
    float Size = 0.0f;
    std::string Type = "taillight256";
    float Offset = 0.0f;
};

struct VehicleSirenDiffuse {
    bool Color = false;
    bool Transparent = false;
};

class VehicleSirenRotator {
public:
    int Direction = 0;
    int Type = 0;
    uint64_t Time = 1000;
    uint64_t TimeElapse = 0;
    float Offset = 0.0f;
    float Radius = 360.0f;

    VehicleSirenRotator(nlohmann::json json) {
        if (json.contains("direction")) {
            if (json["direction"] == "clockwise")
                Direction = 0;
            else if (json["direction"] == "counter-clockwise")
                Direction = 1;
            else if (json["direction"] == "switch")
                Direction = 2;
        }

        if (json.contains("type")) {
            if (json["type"] == "linear")
                Type = 0;
            else if (json["type"] == "ease")
                Type = 1;
        }

        if (json.contains("time"))
            Time = json["time"];

        if (json.contains("offset"))
            Offset = json["offset"];

        if (json.contains("radius"))
            Radius = json["radius"];
    };
};

class VehicleSirenMaterial {
public:
    bool Validate = false;
    float Size = 0.2f;
    float Inertia = 0.0f;
    float InertiaMultiplier = 1.0f;
    float Radius = 180.0f;
    RwRGBA Color = { 255, 255, 255, 128 };
    RwRGBA DefaultColor = { 255, 255, 255, 128 };
    VehicleSirenDiffuse Diffuse;
    int ColorCount = 0;
    uint64_t ColorTime = 0;
    uint64_t ColorTotal = 0;
    std::vector<std::pair<uint64_t, RwRGBA>> Colors;
    int Frames = 0;
    bool State = false;
    bool StateDefault = false;
    std::vector<int> Pattern;
    int PatternCount = 0;
    int PatternTotal = 0;
    uint64_t PatternTime = 0;
    uint64_t Delay = 0;
    VehicleSirenType Type = VehicleSirenType::Directional;
    bool ImVehFt = false;
    VehicleSirenRotator* Rotator;
    VehicleSirenShadow Shadow;

    VehicleSirenMaterial(std::string state, int material, nlohmann::json json);

    bool UpdateMaterial(uint64_t time) {
        if (Pattern.size() == 0)
            return false;

        if ((time - PatternTime) > Pattern[PatternCount]) {
            PatternTime = time;
            PatternCount++;
            State = !State;
            Frames = 0;
            return true;
        }

        return false;
    };

    void ResetMaterial(uint64_t time) {
        PatternCount = 0;
        PatternTime = time;
        State = StateDefault;
        Frames = 0;
        ResetColor(time);
    };

    void ResetColor(uint64_t time) {
        Color = { DefaultColor.red, DefaultColor.green, DefaultColor.blue, DefaultColor.alpha };
        ColorCount = 0;
        ColorTime = time;
    };
};

class VehicleSirenState {
public:
    bool Validate = false;
    std::string Name;
    int Paintjob = -1;
    std::map<int, VehicleSirenMaterial*> Materials;

    VehicleSirenState(std::string state, nlohmann::json json);
};

class VehicleSirenData {
public:
    bool Validate = false;
    std::map<int, std::vector<VehicleMaterial*>> Materials;
    std::vector<VehicleSirenState*> States;

    VehicleSirenData(nlohmann::json json);

    static std::map<std::string, nlohmann::json> References;
    static std::map<std::string, nlohmann::json> ReferenceColors;
};

class VehicleSiren {
public:
    int State = 0;
    bool Mute = false;
    uint64_t Delay = 0;
    std::map<int, std::vector<VehicleDummy*>> Dummies;
    bool SirenState = false;
    bool Trailer = false;

    VehicleSiren(CVehicle* _vehicle);

    bool GetSirenState();

    int GetCurrentState() {
        return State;
    };

private:
    CVehicle* vehicle;
};

class VehicleSirensFeature : public IFeature {
public:
    int CurrentModel = -1;
    std::map<int, VehicleSiren*> vehicleData;
    std::map<int, VehicleSirenData*> modelData;
    std::map<int, std::vector<VehicleDummy*>> modelRotators;

    void Initialize();

private:
    void readSirenConfiguration();
    void readSirenConfigurationIVF();
    void registerSirenConfiguration();
    void createSirenConfiguration(int model, nlohmann::json json);
    void registerMaterial(CVehicle* vehicle, RpMaterial* material, bool ImVehFt = false);
    void enableMaterial(VehicleMaterial* material, VehicleSirenMaterial* mat, uint64_t time);
    void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle, VehicleSirenMaterial* material, eCoronaFlareType type, uint64_t time);
    void enableShadow(CVehicle* vehicle, VehicleDummy* dummy, VehicleSirenMaterial* material, CVector position);
};


extern VehicleSirensFeature VehicleSirens;