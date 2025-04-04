#pragma once
#include "pch.h"
#include <plugin.h>
#include <unordered_map>
#include <functional>

class FeatureMgr {
private:
    struct FrameExtension {
        RwFrame* m_pFrame;
        std::string id;
    };

    static inline std::unordered_map<eModelEntityType, std::unordered_map<void*, std::vector<FrameExtension>>> m_EntityTable;
    static inline std::unordered_map <std::string, std::function<void(void*, RwFrame*, eModelEntityType)>> m_FunctionTable;

    static void FindNodes(void* ptr, RwFrame *frame, eModelEntityType type);

public:
    static void Initialize();

    static void Add(void *ptr, RwFrame* frame, eModelEntityType type);
    static void Process(void *ptr, eModelEntityType type);
    static void Remove(void *ptr, eModelEntityType type);
};