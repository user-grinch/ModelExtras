#include "pch.h"
#include <plugin.h>
#include <windows.h>
#include <shellapi.h>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hWnd);

const char* const LONG_MESSAGE =
"It takes a lot of effort to keep updating with new features and bug fixes. "
"If you find this mod helpful and would like to help with its development, "
"please consider making a donation.\n\n\n"
"This popup only shows up on first installion!";

void ShowDonationWindow()
{
    const char CLASS_NAME[] = "ModelExtrasWindow";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);

    RegisterClass(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = 400;
    int windowHeight = 450;
    int xPos = (screenWidth - windowWidth) / 2;
    int yPos = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW, CLASS_NAME, "ModelExtras Donation Popup", WS_OVERLAPPED | WS_SYSMENU, xPos, yPos, windowWidth,
                    windowHeight, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (hWnd == NULL) return;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            CreateControls(hWnd);
            break;
        case WM_COMMAND:
            Events::initRwEvent += [wParam]() {
                if (LOWORD(wParam) == 1)
                    ShellExecute(NULL, "open", DISCORD_INVITE, NULL, NULL, SW_SHOWNORMAL);
                else if (LOWORD(wParam) == 2)
                    ShellExecute(NULL, "open", PATREON_LINK, NULL, NULL, SW_SHOWNORMAL);
                else if (LOWORD(wParam) == 3)
                    ShellExecute(NULL, "open", GITHUB_LINK, NULL, NULL, SW_SHOWNORMAL);
                };
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CreateControls(HWND hWnd)
{
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, "ModelExtras/logo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    HWND hImage = CreateWindowEx(0, "STATIC", NULL,
                                  WS_CHILD | WS_VISIBLE | SS_BITMAP,
                                  20, 20, 120, 100,
                                  hWnd, NULL, NULL, NULL);
    if (hImage == NULL) {
        MessageBox(NULL, "Failed to create static control for image!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    HWND hTitleText = CreateWindowEx(0, "STATIC", MOD_TITLE, WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 130, 360, 200, hWnd, NULL, NULL, NULL);
    HWND hLongText = CreateWindowEx(0, "STATIC", LONG_MESSAGE, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 180, 360, 200, hWnd, NULL, NULL, NULL);
    HWND hDiscordButton = CreateWindowEx(0, "BUTTON", "Discord", WS_CHILD | WS_VISIBLE | BS_FLAT, 25, 340, 100, 30, hWnd, (HMENU)1, NULL, NULL);
    HWND hDonationButton = CreateWindowEx(0, "BUTTON", "Donate", WS_CHILD | WS_VISIBLE | BS_FLAT, 145, 340, 100, 30, hWnd, (HMENU)2, NULL, NULL);
    HWND hGitHubButton = CreateWindowEx(0, "BUTTON", "GitHub", WS_CHILD | WS_VISIBLE | BS_FLAT, 265, 340, 100, 30, hWnd, (HMENU)3, NULL, NULL);

    SendMessage(hImage, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
    SendMessage(hTitleText, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hLongText, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDiscordButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDonationButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hGitHubButton, WM_SETFONT, (WPARAM)hFont, TRUE);
}