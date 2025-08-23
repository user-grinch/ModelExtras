#include "pch.h"
#include "defines.h"
#include <plugin.h>
#include <windows.h>
#include <shellapi.h>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hWnd);

#if PATRON_BUILD
const char* const LONG_MESSAGE =
    "You're using a donator build, thanks for supporting the project. "
    "Your contribution helps make continued updates, new features, and maintenance possible. "
    "Appreciate your support, and hope you find this build useful.";
#else
const char *const LONG_MESSAGE =
    "Maintaining and enhancing this mod takes a lot of effort and time, with continuous updates, new features, and bug fixes. "
    "If you find this mod valuable and would like to support its ongoing development, "
    "please consider making a donation to help keep the improvements coming.";
#endif

const char *const WARN_MESSAGE =
    "This is NOT a replacement for ImVehFt. Most models should work, but compatibility is not guaranteed.";

void ShowDonationWindow()
{
    const char CLASS_NAME[] = "ModelExtrasWindow";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.hIcon = LoadIcon(NULL, IDI_INFORMATION);

    RegisterClass(&wc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = 450;
    int windowHeight = 600;
    int xPos = (screenWidth - windowWidth) / 2;
    int yPos = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowEx(WS_EX_APPWINDOW, CLASS_NAME, "ModelExtras Startup", WS_OVERLAPPED | WS_SYSMENU, xPos, yPos, windowWidth,
                               windowHeight, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (hWnd == NULL)
        return;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
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
        Events::initRwEvent += [wParam]()
        {
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
    if (!hBitmap)
    {
        MessageBox(NULL, "Failed to load image!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Center the image (256x128)
    int imageWidth = 512;
    int imageHeight = 256;
    int xPos = 90 + (400 - imageWidth) / 2; // Window width is 400, center the image
    int yPos = 20;                          // Set a fixed top position

    HWND hImage = CreateWindowEx(0, "STATIC", NULL,
                                 WS_CHILD | WS_VISIBLE | SS_BITMAP,
                                 xPos, yPos, imageWidth, imageHeight + 40,
                                 hWnd, NULL, NULL, NULL);
    if (hImage == NULL)
    {
        MessageBox(NULL, "Failed to create static control for image!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    HFONT hFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"); // Changed to a more modern font
    HWND hLongText = CreateWindowEx(0, "STATIC", LONG_MESSAGE, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 220, 400, 200, hWnd, NULL, NULL, NULL);
    HWND hWarnText = CreateWindowEx(0, "STATIC", WARN_MESSAGE, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 400, 400, 200, hWnd, NULL, NULL, NULL);
    HWND hDiscordButton = CreateWindowEx(0, "BUTTON", "Discord", WS_CHILD | WS_VISIBLE | BS_FLAT, 25, 500, 100, 30, hWnd, (HMENU)1, NULL, NULL);
    HWND hDonationButton = CreateWindowEx(0, "BUTTON", "Donate", WS_CHILD | WS_VISIBLE | BS_FLAT, 170, 500, 100, 30, hWnd, (HMENU)2, NULL, NULL);
    HWND hGitHubButton = CreateWindowEx(0, "BUTTON", "GitHub", WS_CHILD | WS_VISIBLE | BS_FLAT, 320, 500, 100, 30, hWnd, (HMENU)3, NULL, NULL);

    SendMessage(hImage, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
    SendMessage(hLongText, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hWarnText, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDiscordButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDonationButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hGitHubButton, WM_SETFONT, (WPARAM)hFont, TRUE);
}
