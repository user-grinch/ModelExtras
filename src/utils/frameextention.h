#pragma once
#include <string>
#include <cstdint>

struct RwFrame;
class CVehicle;

constexpr std::string_view PLUGIN_ID_STR = "MEX";
constexpr uint32_t PLUGIN_ID_NUM = 0x42945628;

class RwFrameExtension {
public:
    CVehicle* pOwner;
    RwMatrix* pOrigMatrix;

    static inline uint32_t framePluginOffset;
    static RwFrameExtension* Get(RwFrame* frame);
    static void Initialize();
    
private:
    static RwFrame* Init(RwFrame* pFrame);
    static RwFrame* Shutdown(RwFrame* pFrame);
    static RwFrame* Clone(RwFrame* pCopy, const RwFrame* pSource);
};