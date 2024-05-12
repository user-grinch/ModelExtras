#pragma once
#include "plugin.h"
#include <vector>
#include <map>

class Randomizer {
private:
  static inline std::map<int, std::vector<RwFrame*>> m_pStoredFrames;
  static inline std::map<void*, std::map<std::string, int>> m_pStoredRandoms;
  
public:
  static void Initialize();
  static void Process(void* ptr, RwFrame* frame, eModelEntityType type);
};