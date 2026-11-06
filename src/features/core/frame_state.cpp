#include "pch.h"
#include "frame_state.h"
#include "utils/frameextension.h"

void FeatureFrameState::Capture(RwFrame *frame, bool transform, unsigned depth) {
    if (!frame) return;
    auto *ext = RwFrameExtension::Get(frame);
    if (!ext) return;
    auto found = std::find_if(frames.begin(), frames.end(), [=](const Entry &e) {
        return e.frame == frame && e.generation == ext->generation;
    });
    if (found == frames.end()) {
        frames.push_back({frame, ext->generation, frame->modelling, transform, {}});
        if (!transform) RwFrameForAllObjects(frame, [](RwObject *object, void *context) {
            if (RwObjectGetType(object) == rpATOMIC) {
                auto *atomic = reinterpret_cast<RpAtomic *>(object);
                auto generation = RwFrameExtension::AtomicGeneration(atomic);
                if (generation) static_cast<Entry *>(context)->visibility.emplace_back(atomic, generation, (RpAtomicGetFlags(atomic) & rpATOMICRENDER) != 0);
            }
            return object;
        }, &frames.back());
    }
    if (depth) for (auto *child = frame->child; child; child = child->next) Capture(child, transform, depth - 1);
}

void FeatureFrameState::Restore(CVehicle *vehicle) {
    if (!vehicle || !vehicle->m_pRwClump || frames.empty()) return;
    // Traverse live frames; never dereference a stored pointer to a detached or deleted node.
    const auto visit = [&](auto &&self, RwFrame *frame) -> void {
        if (!frame) return;
        auto *ext = RwFrameExtension::Get(frame);
        if (ext) for (const auto &entry : frames) {
            if (entry.frame != frame || entry.generation != ext->generation) continue;
            if (entry.transform) {
                frame->modelling = entry.matrix;
                RwMatrixUpdate(&frame->modelling);
                RwFrameUpdateObjects(frame);
            } else RwFrameForAllObjects(frame, [](RwObject *object, void *context) {
                if (RwObjectGetType(object) == rpATOMIC) {
                    auto *atomic = reinterpret_cast<RpAtomic *>(object);
                    for (const auto &[saved, generation, visible] : static_cast<const Entry *>(context)->visibility)
                        if (saved == atomic && generation == RwFrameExtension::AtomicGeneration(atomic)) {
                            auto flags = RpAtomicGetFlags(atomic);
                            RpAtomicSetFlags(atomic, visible ? flags | rpATOMICRENDER : flags & ~rpATOMICRENDER);
                            break;
                        }
                }
                return object;
            }, const_cast<Entry *>(&entry));
            break;
        }
        for (auto *child = frame->child; child; child = child->next) self(self, child);
    };
    visit(visit, RpClumpGetFrame(vehicle->m_pRwClump));
}

bool FeatureFrameState::IsCurrent(CVehicle *vehicle, RwFrame *target) const {
    if (!vehicle || !vehicle->m_pRwClump || !target) return false;
    const auto visit = [&](auto &&self, RwFrame *frame) -> bool {
        if (!frame) return false;
        if (frame == target) {
            auto *ext = RwFrameExtension::Get(frame);
            return ext && std::any_of(frames.begin(), frames.end(), [=](const Entry &e) { return e.frame == frame && e.generation == ext->generation; });
        }
        for (auto *child = frame->child; child; child = child->next) if (self(self, child)) return true;
        return false;
    };
    return visit(visit, RpClumpGetFrame(vehicle->m_pRwClump));
}
