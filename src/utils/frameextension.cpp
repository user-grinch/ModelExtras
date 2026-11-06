#include "pch.h"
#include "frameextension.h"
#include "rwplcore.h"

RwFrameExtension* RwFrameExtension::Get(RwFrame* frame) {
    if (frame == nullptr || framePluginOffset == 0 || framePluginOffset == UINT32_MAX)  {
        return nullptr;
    }
    auto address = reinterpret_cast<std::uintptr_t>(frame);
    return reinterpret_cast<RwFrameExtension*>(address + framePluginOffset);
}

uint64_t RwFrameExtension::AtomicGeneration(RpAtomic *atomic) {
    if (!atomic || atomicPluginOffset < 0) return 0;
    return *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(atomic) + atomicPluginOffset);
}

RwFrame* RwFrameExtension::Init(RwFrame* pFrame) {
    if (auto* ext = Get(pFrame)) {
        static uint32_t nextGeneration = 0;
        ext->generation = ++nextGeneration;
        ext->pOwner = nullptr;
        ext->pOrigMatrix = nullptr;
    }
    return pFrame;
}

void RwFrameExtension::Init() {
    Events::attachRwPluginsEvent += []()
    {
        atomicPluginOffset = RpAtomicRegisterPlugin(sizeof(uint32_t), PLUGIN_ID_NUM,
            [](void *object, RwInt32 offset, RwInt32) -> void * {
                static uint32_t nextGeneration = 0;
                *reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(object) + offset) = ++nextGeneration;
                return object;
            }, nullptr, [](void *copy, const void *, RwInt32, RwInt32) -> void * { return copy; });
        framePluginOffset = RwFrameRegisterPlugin(sizeof(RwFrameExtension), PLUGIN_ID_NUM, 
                reinterpret_cast<RwPluginObjectConstructor>(static_cast<RwFrame* (*)(RwFrame*)>(RwFrameExtension::Init)), 
                reinterpret_cast<RwPluginObjectDestructor>(RwFrameExtension::Shutdown), 
                reinterpret_cast<RwPluginObjectCopy>(RwFrameExtension::Clone)
            );
    };
}

RwFrame* RwFrameExtension::Shutdown(RwFrame* pFrame) {
    if (auto* ext = Get(pFrame)) {
        if (ext->pOrigMatrix) {
            delete ext->pOrigMatrix;
            ext->pOrigMatrix = nullptr;
        }
    }
    return pFrame;
}

RwFrame* RwFrameExtension::Clone(RwFrame* pCopy, const RwFrame* pSource) {
    return pCopy;
}