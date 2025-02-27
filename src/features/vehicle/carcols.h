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

struct ColorSet {
    CRGBA primary, secondary, tert, quart;
};

class IVFCarcolsFeature : public IFeature {
private:
    struct VehData {
        bool m_bPri = false, m_bSec = false, m_bTer = false, m_bQuat = false;
        ColorSet m_Colors;
        VehData(CVehicle* pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> ExData;
    static inline std::unordered_map<int, std::vector<ColorSet>> variations;

    static void parseFiles();

public:
    void Initialize();
};

extern IVFCarcolsFeature IVFCarcols;