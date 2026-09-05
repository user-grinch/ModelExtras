#pragma once
#include <cstdint>
#include <bitset>

class InputMgr {
private:
    static inline std::bitset<256> m_CurrentKeys{};
    static inline std::bitset<256> m_PreviousKeys{};
    static inline uint32_t m_LastKeyTimes[256]{};

public:
    static void Update();
    static bool IsKeyDown(int vKey);
    static bool IsKeyJustDown(int vKey);
    static bool IsKeyJustUp(int vKey);
    static bool IsKeyToggled(int vKey, uint32_t delayMs = 300);
};
