#include "overlay.h"
#include "watermark.h"
#include "base64.h"
#include <gdiplus.h>
#include <thread>
#include <iostream>

using namespace Gdiplus;

HINSTANCE hInst;
std::vector<OverlayWindow> overlays;
std::atomic<bool> overlayThreadRunning{ false };

bool enableOverlay = false;
bool enableTextWatermark = false;
bool enableImageWatermark = false;
std::string imageSource;
std::string textSource;

void GenerateAlignedWatermarkGrid(OverlayWindow& overlay, const std::wstring& watermark, HFONT hFont)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

    SIZE textSize = { 0 };
    GetTextExtentPoint32(hdcMem, watermark.c_str(), (int)watermark.length(), &textSize);
    overlay.textSize = textSize;

    int width = overlay.monitorRect.right - overlay.monitorRect.left;
    int height = overlay.monitorRect.bottom - overlay.monitorRect.top;
    int cellW = width / GRID_COLS;
    int cellH = height / GRID_ROWS;

    overlay.watermarkPositions.clear();
    for (int row = 0; row < GRID_ROWS; ++row)
        for (int col = 0; col < GRID_COLS; ++col)
            overlay.watermarkPositions.push_back({
                col * cellW + (cellW - textSize.cx) / 2,
                row * cellH + (cellH - textSize.cy) / 2
                });

    SelectObject(hdcMem, hOldFont);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    overlay.positionsInitialized = true;
}

void UpdateOverlay(const OverlayWindow& overlay) {
    HDC hdcScreen = GetDC(NULL);
    std::wstring watermark = enableTextWatermark ? GenerateWatermarkText(textSource) : L"";
    HBITMAP hBitmap = CreateWatermarkBitmap(overlay, hdcScreen, watermark);
    if (!hBitmap) {
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    int width = overlay.monitorRect.right - overlay.monitorRect.left;
    int height = overlay.monitorRect.bottom - overlay.monitorRect.top;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    SIZE size = { width, height };
    POINT ptSrc = { 0, 0 };
    POINT ptDest = { overlay.monitorRect.left, overlay.monitorRect.top };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(overlay.hwnd, hdcScreen, &ptDest, &size, hdcMem,
        &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void UpdateAllOverlays() {
    for (const auto& overlay : overlays) {
        if (overlay.hwnd) UpdateOverlay(overlay);
    }
}

// Helper: destroy existing overlay windows and free images
void DestroyAllOverlays() {
    for (auto& ov : overlays) {
        if (ov.hwnd) {
            DestroyWindow(ov.hwnd);
            ov.hwnd = nullptr;
        }
        if (ov.pImage) {
            delete ov.pImage;
            ov.pImage = nullptr;
        }
    }
    overlays.clear();
}

// Recreate overlays: destroy current windows, enumerate monitors and create overlays again.
// Must be called from the same thread that owns the windows (we are on the overlay thread).
void RecreateOverlays() {
    // Destroy existing
    DestroyAllOverlays();

    // Re-enumerate monitors and create overlay windows
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    // Force an immediate update for all overlays
    UpdateAllOverlays();

    // Set/reset timer on first overlay (if any)
    if (!overlays.empty()) {
        SetTimer(overlays[0].hwnd, 1, 1000, NULL);
    }
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        UpdateAllOverlays();
        break;
    case WM_DISPLAYCHANGE:
    case WM_DEVICECHANGE:
        RecreateOverlays();  // rebuild overlays dynamically
        break;
    case WM_DESTROY:
        // do NOT call PostQuitMessage — keep overlay thread alive
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT lprc, LPARAM lParam) {
    RECT rc = *lprc;
    const wchar_t CLASS_NAME[] = L"WatermarkOverlay";

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"", WS_POPUP,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);

    if (hwnd) {
        if (enableOverlay)
            SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
        else
            SetWindowDisplayAffinity(hwnd, WDA_NONE);

        OverlayWindow overlay;
        overlay.hwnd = hwnd;
        overlay.monitor = hMonitor;
        overlay.monitorRect = rc;

        // Create font and compute grid positions
        LOGFONT lf = {};
        lf.lfHeight = FONT_HEIGHT;
        lf.lfWeight = FW_NORMAL;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        HFONT hFont = CreateFontIndirect(&lf);
        std::wstring watermark = enableTextWatermark ? GenerateWatermarkText(textSource) : L"";
        GenerateAlignedWatermarkGrid(overlay, watermark, hFont);
        DeleteObject(hFont);

        // Load image once if required (base64 or file)
        if (enableImageWatermark) {
            Gdiplus::Image* pImage = nullptr;
            std::vector<unsigned char> imageData;
            if (is_base64_string(imageSource)) {
                imageData = base64_decode(imageSource);
                if (is_image_data(imageData)) {
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, imageData.size());
                    if (hMem) {
                        void* pData = GlobalLock(hMem);
                        memcpy(pData, imageData.data(), imageData.size());
                        GlobalUnlock(hMem);
                        IStream* pStream = nullptr;
                        if (SUCCEEDED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
                            pImage = Gdiplus::Image::FromStream(pStream);
                            pStream->Release();
                        }
                        GlobalFree(hMem);
                    }
                }
            }
            else {
                // file path
                std::wstring ws(imageSource.begin(), imageSource.end());
                pImage = Gdiplus::Image::FromFile(ws.c_str());
            }

            // Check validity
            if (pImage && pImage->GetLastStatus() == Gdiplus::Ok && pImage->GetWidth() > 0 && pImage->GetHeight() > 0) {
                overlay.pImage = pImage;
            }
            else {
                if (pImage) {
                    delete pImage;
                    pImage = nullptr;
                }
                // fallback: no image available; still keep overlay to allow text watermarks or future reload
                std::wcerr << L"Warning: failed to load watermark image for monitor rect: "
                    << rc.left << L"," << rc.top << L"," << rc.right << L"," << rc.bottom << L"\n";
            }
        }

        ShowWindow(hwnd, SW_SHOW);
        overlays.push_back(overlay);
    }

    return TRUE;
}

void OverlayThreadProc() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    hInst = GetModuleHandle(nullptr);

    const wchar_t CLASS_NAME[] = L"WatermarkOverlay";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    overlays.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    UpdateAllOverlays();

    if (!overlays.empty()) {
        SetTimer(overlays[0].hwnd, 1, 1000, NULL);
    }

    MSG msg = {};
    while (overlayThreadRunning.load()) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                // Ignore WM_QUIT — don't exit due to display reconfiguration
                std::cout << "[Info] Ignoring WM_QUIT (display reconfig in progress)\n";
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Keep overlays refreshed periodically
        UpdateAllOverlays();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    DestroyAllOverlays();

    Gdiplus::GdiplusShutdown(gdiplusToken);
}
