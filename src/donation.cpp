#include "pch.h"
#include "plugin.h"
#include <windows.h>
#include <shellapi.h>

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hWnd);

const char* const LONG_MESSAGE =
    "It takes a lot of effort to keep updating with new features and bug fixes."
    "If you find this mod helpful and would like to help with its development, "
    "please consider making a donation.\n\n\n"
    "This popup only shows up on first installion!";

void ShowDonationWindow()
{
    // Register the window class.
    const char CLASS_NAME[] = "DonationWindowClass";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);

    RegisterClass(&wc);

    // Create the window.
    HWND hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,            // Extended window styles.
        CLASS_NAME,                 // Window class
        "ModelExtras Donation Popup",             // Window text
        WS_OVERLAPPED | WS_SYSMENU, // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 450, // Size and position
        NULL,                       // Parent window
        NULL,                       // Menu
        GetModuleHandle(NULL),      // Instance handle
        NULL                        // Additional application data
    );

    if (hWnd == NULL)
    {
        return;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Run the message loop.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Window procedure
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

// Function to create controls
void CreateControls(HWND hWnd)
{
    // Load the image
    HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, "ModelExtras/logo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    // Create a static control for the image
    HWND hImage = CreateWindowEx(0, "STATIC", NULL,
                                  WS_CHILD | WS_VISIBLE | SS_BITMAP,
                                  20, 20, 120, 100,
                                  hWnd, NULL, NULL, NULL);
    if (hImage == NULL) {
        MessageBox(NULL, "Failed to create static control for image!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Set the bitmap to the static control
    SendMessage(hImage, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

    // Create a static control for the long message
    // Create a font for Consolas
    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Consolas");

    // Create a static control for the long message
    HWND hTitleText = CreateWindowEx(0, "STATIC", MOD_TITLE,
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    20, 130, 360, 200,
                    hWnd, NULL, NULL, NULL);

    HWND hLongText = CreateWindowEx(0, "STATIC", LONG_MESSAGE,
                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                    20, 180, 360, 200,
                    hWnd, NULL, NULL, NULL);

    // Set Consolas font to the static control
    SendMessage(hTitleText, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hLongText, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hDiscordButton = CreateWindowEx(0, "BUTTON", "Discord",
                    WS_CHILD | WS_VISIBLE | BS_FLAT,
                    25, 340, 100, 30,
                    hWnd, (HMENU)1, NULL, NULL);

    HWND hDonationButton = CreateWindowEx(0, "BUTTON", "Donate",
                    WS_CHILD | WS_VISIBLE | BS_FLAT,
                    145, 340, 100, 30,
                    hWnd, (HMENU)2, NULL, NULL);
    
    HWND hGitHubButton = CreateWindowEx(0, "BUTTON", "GitHub",
                    WS_CHILD | WS_VISIBLE | BS_FLAT,
                    265, 340, 100, 30,
                    hWnd, (HMENU)3, NULL, NULL);
    
    SendMessage(hDiscordButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDonationButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hGitHubButton, WM_SETFONT, (WPARAM)hFont, TRUE);
}
