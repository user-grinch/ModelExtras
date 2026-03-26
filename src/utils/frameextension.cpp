#include "pch.h"
#include "frameextention.h"
#include "rwplcore.h"

RwFrameExtension* RwFrameExtension::Get(RwFrame* frame) {
    if (frame == nullptr)  {
        return nullptr;
    }
    auto address = reinterpret_cast<std::uintptr_t>(frame);
    return reinterpret_cast<RwFrameExtension*>(address + framePluginOffset);
}

RwFrame* RwFrameExtension::Init(RwFrame* pFrame) {
    if (auto* ext = Get(pFrame)) {
        ext->pOwner = nullptr;
        ext->pOrigMatrix = nullptr;
    }
    return pFrame;
}

void RwFrameExtension::Initialize() {
    Events::attachRwPluginsEvent += []()
    {
        framePluginOffset = RwFrameRegisterPlugin(sizeof(RwFrameExtension), PLUGIN_ID_NUM, 
                reinterpret_cast<RwPluginObjectConstructor>(RwFrameExtension::Init), 
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