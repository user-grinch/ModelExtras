#pragma once
#include <rwcore.h>
#include <vector>

typedef class CVehicle CVehicle;
typedef enum class eModelEntityType eModelEntityType;

class FrameUtil {
public:
    static bool IsOkAtomicVisible(RwFrame* frame);
    static RwFrame * Clone(RwFrame *frame, RpClump *clump, RwFrame *parent, bool isRoot);

    // Returns the number of childs a parent contains
    static unsigned int GetChildCount(RwFrame *pParent);

    // Rotate model pFrame
    static void SetRotationX(RwFrame *pFrame, float angle);
    static void SetRotationY(RwFrame *pFrame, float angle);
    static void SetRotationZ(RwFrame *pFrame, float angle);

    // Stores all the childs in a vector
    static void StoreChilds(RwFrame *pParent, std::vector<RwFrame *> &pFrame);

    static void HideAllAtomics(RwFrame *pFrame);
    static void ShowAllAtomics(RwFrame *pFrame);

    static void HideChildWithName(RwFrame *pParent, const char *name);
    static void ShowChildWithName(RwFrame *pParent, const char *name);
    static void HideAllChilds(RwFrame *pParent);
    static void ShowAllChilds(RwFrame *pParent);
};