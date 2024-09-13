#pragma once
#include "pch.h"
#include <string>
#include <map>

/*  AVS Paintjobs Feature
    This is mostly here for backwards compatibility
    Better to use the Random Remap function instead
*/
class PaintJobs {
private:
    // Stores model specific paintjob data
    static inline std::unordered_map<int, std::map<int, int>> m_PaintjobStore;

    // Stores whether a vehicle has been painted or not
    static inline std::unordered_map<int, bool> m_PaintedFlag;

    static void Load();

public:
    static void Initialize();
};