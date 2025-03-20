#include "pch.h"
#include "GrinchTrainerAPI.h"

class VehicleFrameView
{
    struct FrameNode
    {
        const char *name;
        FrameNode *children;
        FrameNode *next;
    };

    static inline std::unordered_map<int, FrameNode *> cachedTrees;

    static FrameNode *BuildFrameHierarchy(RwFrame *frame, int depth = 0)
    {
        if (!frame)
            return nullptr;

        FrameNode *node = new FrameNode{GetFrameNodeName(frame), nullptr, nullptr};
        FrameNode **lastChild = &node->children;

        for (RwFrame *child = frame->child; child; child = child->next)
        {
            *lastChild = BuildFrameHierarchy(child, depth + 1);
            if (*lastChild)
                lastChild = &(*lastChild)->next;
        }

        return node;
    }

    static void UpdateCache(CVehicle *vehicle)
    {
        if (!vehicle)
            return;
        RwFrame *rootFrame = vehicle->m_pRwClump ? (RwFrame *)vehicle->m_pRwClump->object.parent : nullptr;
        cachedTrees[vehicle->m_nModelIndex] = BuildFrameHierarchy(rootFrame);
    }

    static void DrawTextHierarchy(FrameNode *node)
    {
        while (node)
        {
            TAPI_Text(node->name);
            DrawTextHierarchy(node->children);
            node = node->next;
        }
    }

public:
    static void Render(CVehicle *vehicle)
    {
        if (!vehicle)
            return;

        if (cachedTrees.find(vehicle->m_nModelIndex) == cachedTrees.end())
            UpdateCache(vehicle);

        DrawTextHierarchy(cachedTrees[vehicle->m_nModelIndex]);
    }
};