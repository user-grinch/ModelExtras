#pragma once
#include "pch.h"

#include <functional>

class RenderHook
{
  private:
    static inline WNDPROC ogWndProc = nullptr;
    static inline bool bMouseVisible = false;
    static inline bool bInitialized = false;
    static inline std::function<void()> renderFn = nullptr;

    // Hooks
    static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void ProcessMouse();
    static void InitImGui();
    static void RenderImGui();
    static void ShutdownImGui();

  public:
    static bool IsCursorVisible();
    static void SetCursorVisible(bool visible);

    static void Init(std::function<void()> callback);
    static void Shutdown();
};
