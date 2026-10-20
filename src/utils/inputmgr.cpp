#include "pch.h"
#include "inputmgr.h"
#include "samp.h"
#include "util.h"
#include "gui/studio.h"
#include <CTimer.h>

void InputMgr::Update() {
    m_PreviousKeys = m_CurrentKeys;

    if (!Util::IsWindowFocused() || SAMP::IsInputActive()) {
        m_CurrentKeys.reset();
        return;
    }

    for (int i = 0; i < 256; ++i) {
        bool down = (GetAsyncKeyState(i) & 0x8000) != 0;
        if (!down) m_BlockedKeys[i] = false;
        else if (Studio::IsOpen()) m_BlockedKeys[i] = true;
        m_CurrentKeys[i] = down && !m_BlockedKeys[i] && !Studio::IsOpen();
    }
    // The menu shortcut must never also trigger the left indicator.
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) {
        m_BlockedKeys['Z'] = (GetAsyncKeyState('Z') & 0x8000) != 0;
        m_CurrentKeys['Z'] = false;
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
