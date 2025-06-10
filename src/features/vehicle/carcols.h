#pragma once
#include <plugin.h>
#include "../../interface/ifeature.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

struct ColorSet
{
    CRGBA primary, secondary, tert, quart;
};

class IVFCarcols
{
private:
    struct VehData
    {
        bool m_bPri = false, m_bSec = false, m_bTer = false, m_bQuat = false;
        ColorSet m_Colors;
        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };
    static inline bool m_bEnabled = false;
    static inline VehicleExtendedData<VehData> ExData;
    static inline std::unordered_map<int, std::vector<ColorSet>> variations;

public:
    static void Initialize();
    static void Parse(const nlohmann::json &data, int model);
    static CRGBA GetColor(CVehicle *pVeh, RpMaterial *pMat, CRGBA col);
};