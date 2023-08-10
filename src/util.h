#pragma once
#include "plugin.h"

class Util {
  public:
    // Returns value of regex from source string
    static std::string GetRegexVal(const std::string& src, const std::string&& ptrn, const std::string&& def);

    // Returns the number of childs a parent contains
    static uint32_t GetChildCount(RwFrame* parent);

    // Returns the speed of the vehicle handler
    static float GetVehicleSpeedRealistic(CVehicle * vehicle);

    // Rotate model frame
    static void RotateFrameX(RwFrame* frame, float angle);
    static void RotateFrameY(RwFrame* frame, float angle);
    static void RotateFrameZ(RwFrame* frame, float angle);

    // Stores all the childs in a vector
    static void StoreChilds(RwFrame * parent_frame, std::vector<RwFrame*>& frame);

    static void HideAllAtomics(RwFrame * frame);
    static void ShowAllAtomics(RwFrame * frame);

    static void HideChildWithName(RwFrame *parent_frame, const char* name);
    static void ShowChildWithName(RwFrame *parent_frame, const char* name);
    static void HideAllChilds(RwFrame *parent_frame);
    static void ShowAllChilds(RwFrame *parent_frame);
};

