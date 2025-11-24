#include "storage_blocker.h"
#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

bool SB_IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE hToken = nullptr;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;

    TOKEN_ELEVATION elevation{};
    DWORD dwSize = 0;

    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        isAdmin = elevation.TokenIsElevated;

    CloseHandle(hToken);
    return isAdmin == TRUE;
}

bool SB_GetPropertyString(HDEVINFO hDevInfo,
    SP_DEVINFO_DATA& devInfoData,
    DWORD property,
    std::wstring& out)
{
    WCHAR buffer[256];
    DWORD dataType = 0;
    DWORD size = sizeof(buffer);

    if (!SetupDiGetDeviceRegistryPropertyW(
        hDevInfo, &devInfoData, property, &dataType,
        reinterpret_cast<PBYTE>(buffer), size, &size))
        return false;

    if (dataType != REG_SZ && dataType != REG_EXPAND_SZ)
        return false;

    out.assign(buffer);
    return true;
}

bool SB_IsStorageDevice(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfoData)
{
    std::wstring serviceName;
    std::wstring className;

    SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_SERVICE, serviceName);
    SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_CLASS, className);

    auto toUpper = [](std::wstring& s) {
        for (auto& ch : s) ch = towupper(ch);
        };

    toUpper(serviceName);
    toUpper(className);

    bool isUsbStorage = (serviceName == L"USBSTOR");
    bool isWpdDevice = (className == L"WPD");

    return isUsbStorage || isWpdDevice;
}

void SB_DisableAllStorageDevices()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(
        nullptr, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_ALLCLASSES);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    std::wcout << L"[SCAN] Disabling storage devices...\n";

    for (DWORD index = 0;
        SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData);
        ++index)
    {
        if (!SB_IsStorageDevice(hDevInfo, devInfoData))
            continue;

        DEVINST devInst = devInfoData.DevInst;
        CONFIGRET cr = CM_Disable_DevNode(devInst, 0);

        std::wstring serviceName, className;
        SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_SERVICE, serviceName);
        SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_CLASS, className);

        if (cr == CR_SUCCESS)
        {
            std::wcout << L"  [DISABLED] " << serviceName
                << L" | " << className << L"\n";
        }
        else
        {
            std::wcout << L"  [FAILED] " << serviceName
                << L" | " << className
                << L" (Err: " << cr << L")\n";
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

void SB_EnableAllStorageDevices()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(
        nullptr, nullptr, nullptr,
        DIGCF_ALLCLASSES);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return;

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    std::wcout << L"[SCAN] Enabling disabled storage devices...\n";

    for (DWORD index = 0;
        SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData);
        ++index)
    {
        if (!SB_IsStorageDevice(hDevInfo, devInfoData))
            continue;

        DEVINST devInst = devInfoData.DevInst;

        ULONG status = 0, problemCode = 0;
        CONFIGRET crStat = CM_Get_DevNode_Status(&status, &problemCode, devInst, 0);

        if (crStat != CR_SUCCESS)
            continue;

        if (problemCode != CM_PROB_DISABLED)
            continue;

        CONFIGRET cr = CM_Enable_DevNode(devInst, 0);

        std::wstring serviceName, className;
        SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_SERVICE, serviceName);
        SB_GetPropertyString(hDevInfo, devInfoData, SPDRP_CLASS, className);

        if (cr == CR_SUCCESS)
        {
            std::wcout << L"  [ENABLED] " << serviceName
                << L" | " << className << L"\n";
        }
        else
        {
            std::wcout << L"  [FAILED] " << serviceName
                << L" | " << className
                << L" (Err: " << cr << L")\n";
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

LRESULT CALLBACK SB_HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DEVICECHANGE && wParam == DBT_DEVNODES_CHANGED)
    {
        std::wcout << L"[EVENT] Device change detected. Re-disabling...\n";
        SB_DisableAllStorageDevices();
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SB_RunMonitorLoop()
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SB_HiddenWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"StorageBlockerHiddenClass";

    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", 0,
        0, 0, 0, 0,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd)
    {
        std::wcerr << L"[ERROR] Cannot create monitor window.\n";
        return;
    }

    std::wcout << L"[MONITOR] Waiting for device changes...\n";

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

/* High-level API for main.cpp */

void SB_DisableAndMonitor()
{
    SB_DisableAllStorageDevices();
    SB_RunMonitorLoop();
}

void SB_EnableOnly()
{
    SB_EnableAllStorageDevices();
}
