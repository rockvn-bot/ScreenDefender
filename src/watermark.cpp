#include "watermark.h"
#include "base64.h"
#include <gdiplus.h>
#include <fstream>
#include <iostream>
#include <cmath>

using namespace Gdiplus;

extern bool enableImageWatermark;
extern bool enableTextWatermark;
extern std::string imageSource;

std::wstring GenerateWatermarkText(const std::string& str) {
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
        (int)str.size(), nullptr, 0);
    std::wstring waterText(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
        (int)str.size(), &waterText[0], size_needed);
    return waterText;
}

// Creates a 32-bit ARGB HBITMAP for the overlay using the cached overlay.pImage if present
HBITMAP CreateWatermarkBitmap(const OverlayWindow& overlay, HDC hdcScreen, const std::wstring& watermark) {
    int width = overlay.monitorRect.right - overlay.monitorRect.left;
    int height = overlay.monitorRect.bottom - overlay.monitorRect.top;
    if (width <= 0 || height <= 0) return nullptr;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    RECT rect = { 0, 0, width, height };
    // Fill with fully transparent (alpha=0) background
    // For DIBSection with BI_RGB, memory is BGRX; we'll zero it so alpha is 0 when blended.
    if (pBits) {
        ZeroMemory(pBits, width * height * 4);
    }
    else {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdcMem, &rect, hBrush);
        DeleteObject(hBrush);
    }

    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, WATERMARK_COLOR);

    if (enableImageWatermark && overlay.pImage) {
        // draw via GDI+ onto hdcMem
        Gdiplus::Graphics graphics(hdcMem);
        int drawW = 64, drawH = 64; // scale down
        for (const auto& pt : overlay.watermarkPositions) {
            int cx = pt.x + overlay.textSize.cx / 2;
            int cy = pt.y + overlay.textSize.cy / 2;
            graphics.DrawImage(overlay.pImage, cx - drawW / 2, cy - drawH / 2, drawW, drawH);
        }
    }
    else if (enableTextWatermark && !watermark.empty()) {
        LOGFONT lf = {};
        lf.lfHeight = FONT_HEIGHT;
        lf.lfWeight = FW_NORMAL;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        HFONT hFont = CreateFontIndirect(&lf);
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

        SetGraphicsMode(hdcMem, GM_ADVANCED);

        for (const auto& pt : overlay.watermarkPositions) {
            XFORM xForm = { 0 };
            float angle = 45.0f * 3.14159265f / 180.0f;
            float cosA = cosf(angle);
            float sinA = sinf(angle);

            int cx = pt.x + overlay.textSize.cx / 2;
            int cy = pt.y + overlay.textSize.cy / 2;

            xForm.eM11 = cosA;
            xForm.eM12 = sinA;
            xForm.eM21 = -sinA;
            xForm.eM22 = cosA;
            xForm.eDx = (FLOAT)cx;
            xForm.eDy = (FLOAT)cy;

            SetWorldTransform(hdcMem, &xForm);
            TextOut(hdcMem, -overlay.textSize.cx / 2, -overlay.textSize.cy / 2, watermark.c_str(), (int)watermark.length());
            XFORM identity = { 1, 0, 0, 1, 0, 0 };
            SetWorldTransform(hdcMem, &identity);
        }

        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);
    }

    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);

    return hBitmap;
}
