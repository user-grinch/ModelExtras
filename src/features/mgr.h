#pragma once
#include <plugin.h>
#include <vector>
#include <map>
#include <functional>

class FeatureMgr {
private:
    struct FrameExtension {
        RwFrame* m_pFrame;
        std::string id;
    };

    // May need to switch to entity based table later
    static inline std::map<int, std::vector<FrameExtension>> m_ModelTable;
    static inline std::map <std::string, std::function<void(void*, RwFrame*, eModelEntityType)>> m_FunctionTable;

    static void FindNodes(void* ptr, RwFrame *frame, eModelEntityType type);

public:
    static void Initialize();

    static void Add(void *ptr, RwFrame* frame, eModelEntityType type);
    static void Process(void *ptr, eModelEntityType type);
    static void Remove(void *ptr, eModelEntityType type);
};