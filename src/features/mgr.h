#pragma once
#include <plugin.h>
#include <vector>
#include <map>
#include <functional>

class FeatureManager {
private:
    struct FrameExtension {
        RwFrame* ptr;
        std::string id;
    };

    // May need to switch to entity based table later
    std::map<int, std::vector<FrameExtension>> m_ModelTable;
    std::map <std::string, std::function<void(RwFrame*, CEntity*)>> m_FunctionTable;

    void FindNodes(RwFrame * frame, CEntity* pEntity);

public:
    FeatureManager();

    void Initialize(CEntity *pEntity, RwFrame* frame);
    void Process(CEntity *pEntity);
    void Remove(CEntity *pEntity);
};

extern FeatureManager FeatureMgr;