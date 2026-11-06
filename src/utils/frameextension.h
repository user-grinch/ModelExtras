#pragma once
#include <string>
#include <cstdint>
#include "defines.h"

struct RwFrame;
struct RpAtomic;
class CVehicle;

class RwFrameExtension {
public:
    uint32_t generation;
    CVehicle* pOwner;
    RwMatrix* pOrigMatrix;

    static inline uint32_t framePluginOffset;
    static inline int atomicPluginOffset = -1;
    static uint64_t AtomicGeneration(RpAtomic *atomic);
    static RwFrameExtension* Get(RwFrame* frame);
    static void Init();
    
private:
    static RwFrame* Init(RwFrame* pFrame);
    static RwFrame* Shutdown(RwFrame* pFrame);
    static RwFrame* Clone(RwFrame* pCopy, const RwFrame* pSource);
};
