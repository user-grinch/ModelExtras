#pragma once
#include <plugin.h>
#include <vector>
#include <map>
#include <functional>

class FeatureManager {
private:
    struct FrameExtension {
        RwFrame* m_pFrame;
        std::string id;
    };

    // May need to switch to entity based table later
    std::map<int, std::vector<FrameExtension>> m_ModelTable;
    std::map <std::string, std::function<void(void*, RwFrame*, eModelEntityType)>> m_FunctionTable;

    void FindNodes(void* ptr, RwFrame *frame, eModelEntityType type);

public:
    FeatureManager();

    void Initialize(void *ptr, RwFrame* frame, eModelEntityType type);
    void Process(void *ptr, eModelEntityType type);
    void Remove(void *ptr, eModelEntityType type);
};

extern FeatureManager FeatureMgr;