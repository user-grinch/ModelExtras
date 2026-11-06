#pragma once
#include <RenderWare.h>
#include <vector>
#include <cstdint>
#include <tuple>
class CVehicle;

// Captured during node discovery, restored only on a feature transition.
struct FeatureFrameState {
    struct Entry {
        RwFrame *frame;
        uint64_t generation;
        RwMatrix matrix;
        bool transform;
        std::vector<std::tuple<RpAtomic *, uint64_t, bool>> visibility;
    };
    std::vector<Entry> frames;
    explicit FeatureFrameState(CVehicle * = nullptr) {}
    void Capture(RwFrame *frame, bool transform = true, unsigned depth = 0);
    void Restore(CVehicle *vehicle);
    bool IsCurrent(CVehicle *vehicle, RwFrame *frame) const;
};
