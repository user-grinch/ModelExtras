#pragma once
#include <rwcore.h>
#include <vector>

using eModelEntityType = enum class eModelEntityType;

enum eVisibilityStatus {
    Ok = 1,
    Damaged,
    Missing
};

class FrameUtil {
public:
    static bool IsAtomicVisible(RwFrame* frame);
    static bool IsOkAtomicVisible(RwFrame* frame);

    static void DestroyNodeHierarchyRecursive(RwFrame * frame);

    // Rotate model pFrame
    static void SetRotationX(RwFrame *pFrame, float angle);
    static void SetRotationY(RwFrame *pFrame, float angle);

    // Stores all the childs in a vector
    static void StoreChilds(RwFrame *pParent, std::vector<RwFrame *> &pFrame);

    static void HideAllAtomics(RwFrame *pFrame);
    static void ShowAllAtomics(RwFrame *pFrame);
    static void HideAllAtomicsExcept(RwFrame *frame, int indexToKeep);

    static void HideAllChilds(RwFrame *pParent);
    static void ShowAllChilds(RwFrame *pParent);
};