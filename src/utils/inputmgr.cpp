#include "pch.h"
#include "inputmgr.h"
#include "samp.h"
#include "util.h"
#include <CTimer.h>

void InputMgr::Update() {
    m_PreviousKeys = m_CurrentKeys;

    if (!Util::IsWindowFocused() || SAMP::IsInputActive()) {
        m_CurrentKeys.reset();
        return;
    }

    for (int i = 0; i < 256; ++i) {
        m_CurrentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}

bool InputMgr::IsKeyDown(int vKey) {
    if (vKey < 0 || vKey >= 256) return false;
    return m_CurrentKeys[vKey];
}

bool InputMgr::IsKeyJustDown(int vKey) {
    if (vKey < 0 || vKey >= 256) return false;
    return m_CurrentKeys[vKey] && !m_PreviousKeys[vKey];
}

bool InputMgr::IsKeyJustUp(int vKey) {
    if (vKey < 0 || vKey >= 256) return false;
    return !m_CurrentKeys[vKey] && m_PreviousKeys[vKey];
}

bool InputMgr::IsKeyToggled(int vKey, uint32_t delayMs) {
    if (vKey < 0 || vKey >= 256) return false;
    if (!m_CurrentKeys[vKey]) return false;

    if (!m_PreviousKeys[vKey]) {
        m_LastKeyTimes[vKey] = CTimer::m_snTimeInMilliseconds;
        return true;
    }

    if (delayMs > 0 && (CTimer::m_snTimeInMilliseconds - m_LastKeyTimes[vKey]) >= delayMs) {
        m_LastKeyTimes[vKey] = CTimer::m_snTimeInMilliseconds;
        return true;
    }

    return false;
}
