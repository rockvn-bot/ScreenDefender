#pragma once
#ifndef STORAGE_BLOCKER_H
#define STORAGE_BLOCKER_H

#include <string>

bool SB_IsRunningAsAdmin();
void SB_DisableAllStorageDevices();
void SB_EnableAllStorageDevices();
void SB_RunMonitorLoop();

// High-level API for main program
void SB_DisableAndMonitor();
void SB_EnableOnly();

#endif
