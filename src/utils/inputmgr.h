#pragma once
#include <cstdint>
#include <bitset>

class InputMgr {
private:
    static inline std::bitset<256> m_CurrentKeys{};
    static inline std::bitset<256> m_PreviousKeys{};
    static inline std::bitset<256> m_BlockedKeys{};

public:
    static void Update();
    static bool IsKeyDown(int vKey);
    static bool IsKeyJustDown(int vKey);
};
