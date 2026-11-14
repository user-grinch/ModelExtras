#pragma once
#include "studio_support.h"
#include <string>
#include <vector>

class CVehicleModelInfo;

namespace StudioModel {
using namespace StudioSupport;

void Light(Model &out, eMaterialType type, unsigned flags);
Model Inspect(CVehicleModelInfo *info, int model, std::vector<std::string> &frames);

} // namespace StudioModel
