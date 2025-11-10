#pragma once
#include <windows.h>
#include <string>
#include "overlay.h"

// Function that generates watermark text
std::wstring GenerateWatermarkText(const std::string& str);

// Create the watermark bitmap for a given overlay
HBITMAP CreateWatermarkBitmap(const OverlayWindow& overlay, HDC hdcScreen, const std::wstring& watermark);
