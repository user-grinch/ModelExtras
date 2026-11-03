#pragma once
#include <string>
#include <cstdint>
#include "defines.h"

struct RwFrame;
class CVehicle;

class RwFrameExtension {
public:
    CVehicle* pOwner;
    RwMatrix* pOrigMatrix;

    static inline uint32_t framePluginOffset;
    static RwFrameExtension* Get(RwFrame* frame);
    static void Init();
    
private:
    static RwFrame* Init(RwFrame* pFrame);
    static RwFrame* Shutdown(RwFrame* pFrame);
    static RwFrame* Clone(RwFrame* pCopy, const RwFrame* pSource);
};
