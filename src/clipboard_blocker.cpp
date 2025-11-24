#include "clipboard_blocker.h"
#include <windows.h>
#include <iostream>

static HWND g_hWnd = nullptr;
static HWND g_nextViewer = nullptr;
static bool g_blocking = false;

LRESULT CALLBACK ClipboardBlockerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DRAWCLIPBOARD:   // Clipboard changed event
        if (g_blocking)
        {
            if (OpenClipboard(nullptr))
            {
                EmptyClipboard();
                CloseClipboard();
                std::cout << "[CLIPBOARD] Blocked clipboard change.\n";
            }
        }

        // Pass event to next viewer
        if (g_nextViewer)
            SendMessage(g_nextViewer, msg, wParam, lParam);
        return 0;

    case WM_CHANGECBCHAIN:
        if ((HWND)wParam == g_nextViewer)
            g_nextViewer = (HWND)lParam;
        else if (g_nextViewer)
            SendMessage(g_nextViewer, msg, wParam, lParam);
        return 0;

    case WM_DESTROY:
        ChangeClipboardChain(hwnd, g_nextViewer);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CB_CreateHiddenWindow()
{
    if (g_hWnd) return; // already created

    WNDCLASSA wc{};
    wc.lpfnWndProc = ClipboardBlockerWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "ClipboardBlockerHiddenWindow";

    RegisterClassA(&wc);

    g_hWnd = CreateWindowA(
        wc.lpszClassName,
        "",
        0,
        0, 0, 0, 0,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    g_nextViewer = SetClipboardViewer(g_hWnd);
}

DWORD WINAPI CB_MonitorThread(LPVOID)
{
    CB_CreateHiddenWindow();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void CB_StartBlocking()
{
    if (g_blocking) return; // already running

    g_blocking = true;

    // Start monitoring thread
    CreateThread(nullptr, 0, CB_MonitorThread, nullptr, 0, nullptr);

    std::cout << "[CLIPBOARD] Blocking ENABLED.\n";
}

void CB_StopBlocking()
{
    g_blocking = false;
    std::cout << "[CLIPBOARD] Blocking DISABLED.\n";
}
