#pragma once
#include <windows.h>
#include <vector>
#include <atomic>
#include <string>
#include <gdiplus.h>

// Constants
const int GRID_COLS = 5;
const int GRID_ROWS = 5;
const int FONT_HEIGHT = -24;
const COLORREF WATERMARK_COLOR = RGB(128, 128, 128);

// Overlay window info
struct OverlayWindow {
    HWND hwnd = nullptr;
    HMONITOR monitor = nullptr;
    RECT monitorRect = {};
    std::vector<POINT> watermarkPositions;
    SIZE textSize = {};
    bool positionsInitialized = false;

    // cached GDI+/image pointer for image watermark (owned by overlay)
    Gdiplus::Image* pImage = nullptr;
};

// Globals (declared here, defined in main.cpp)
extern HINSTANCE hInst;
extern std::vector<OverlayWindow> overlays;
extern std::atomic<bool> overlayThreadRunning;

// Config flags
extern bool enableOverlay;
extern bool enableTextWatermark;
extern bool enableImageWatermark;
extern std::string imageSource;
extern std::string textSource;

// Functions
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprc, LPARAM);
void GenerateAlignedWatermarkGrid(OverlayWindow& overlay, const std::wstring& watermark, HFONT hFont);
void UpdateOverlay(const OverlayWindow& overlay);
void UpdateAllOverlays();
void OverlayThreadProc();
