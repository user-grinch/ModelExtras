#include "pch.h"

#include <CMenuManager.h>
#include <imgui_impl_win32.h>

#include "imgui_impl_rw.h"
#include "renderhook.h"

using namespace plugin;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool RenderHook::IsCursorVisible()
{
    return bMouseVisible;
}

void RenderHook::SetCursorVisible(bool visible)
{
    bMouseVisible = visible;
}

LRESULT RenderHook::WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    if (ImGui::GetIO().WantTextInput)
    {
        Call<0x53F1E0>();
        return 1;
    }
    return CallWindowProc(ogWndProc, hWnd, uMsg, wParam, lParam);
}

float UpdateScaling(HWND hwnd)
{
    float scale = std::min(screen::GetScreenHeight() / 1080, screen::GetScreenWidth() / 1920);
    ImGui::GetStyle().ScaleAllSizes(std::ceil(scale));
    return scale * 20.0f;
}

void RenderHook::RenderImGui()
{
    static bool imguiInitialized = false;

    if (!imguiInitialized)
    {
        InitImGui();
        imguiInitialized = true;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!FrontEndMenuManager.m_bMenuActive)
    {
        ProcessMouse();

        ImGui_ImplWin32_NewFrame();
        ImGui_ImplRW_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(NULL);

        if (renderFn)
        {
            renderFn();
        }

        ImGui::PopFont();

        io.MouseDrawCursor = bMouseVisible;
        io.ConfigDebugHighlightIdConflicts = false;

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplRW_RenderDrawData(ImGui::GetDrawData());
    }
    else
    {
        bool temp = bMouseVisible;
        io.MouseDrawCursor = bMouseVisible = false;
        ProcessMouse();
        bMouseVisible = temp;
    }
}

void RenderHook::InitImGui()
{
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(RsGlobal.ps->window);
    ImGui_ImplRW_Init();
    ImGui_ImplWin32_EnableDpiAwareness();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    // io.FontDefault = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
    patch::Nop(0x00531155, 5); // shift trigger fix
}

void RenderHook::ShutdownImGui()
{
    if (ogWndProc)
    {
        SetWindowLongPtr(RsGlobal.ps->window, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(ogWndProc));
        ogWndProc = nullptr;
    }
    ImGui_ImplRW_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void RenderHook::ProcessMouse()
{
    static bool lastMouseState = false;

    bool isController = patch::Get<BYTE>(0xBA6818);
    if (/*gEditorVisible && */ isController && bMouseVisible)
    {
        if (CPlayerPed *player = FindPlayerPed())
        {
            if (CPad *pad = player->GetPadFromPlayer())
            {
                pad->DisablePlayerControls = bMouseVisible;
            }
        }
    }

    if (lastMouseState == bMouseVisible)
    {
        return;
    }

    if (bMouseVisible)
    {
        patch::SetUChar(0x6194A0, 0xC3); // psSetMousePos
        patch::Nop(0x541DD7, 5);         // disable UpdateMouse
        patch::SetUChar(0x4EB731, 0xEB); // skip mouse checks
        patch::SetUChar(0x4EB75A, 0xEB);
    }
    else
    {
        patch::SetUChar(0x6194A0, 0xE9);
        patch::SetRaw(0x541DD7, (void *)"\xE8\xE4\xD5\xFF\xFF", 5);
        patch::SetUChar(0x4EB731, 0x74); // restore jz
        patch::SetUChar(0x4EB75A, 0x74);
    }

    CPad::UpdatePads();
    CPad::NewMouseControllerState.x = 0;
    CPad::NewMouseControllerState.y = 0;
    CPad::ClearMouseHistory();

    if (auto pad = CPad::GetPad(0))
    {
        pad->NewState.DPadUp = pad->OldState.DPadUp = 0;
        pad->NewState.DPadDown = pad->OldState.DPadDown = 0;
    }

    lastMouseState = bMouseVisible;
}

void RenderHook::Init(std::function<void()> callback)
{
    if (bInitialized)
    {
        return;
    }

    static CdeclEvent<AddressList<0x53EB12, H_CALL>, PRIORITY_AFTER,
                              ArgPickNone, void()>
        draw2dStuffEvent;
    draw2dStuffEvent += []() { RenderImGui(); };

    Events::drawMenuBackgroundEvent += []() { RenderImGui(); };

    renderFn = std::move(callback);
    ogWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(RsGlobal.ps->window, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));
    bInitialized = true;
}

void RenderHook::Shutdown()
{
    if (!bInitialized)
    {
        return;
    }

    renderFn = nullptr;
    ShutdownImGui();
    bInitialized = false;
}
