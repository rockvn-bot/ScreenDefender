#pragma once
#ifndef DLP_LOGGER_H
#define DLP_LOGGER_H

#include <string>

#define DLP_LOG_DIRECTORY "C:/ProgramData/AmZetta/logs/dlp"
#define DLP_LOG_FILE  "C:/ProgramData/AmZetta/logs/dlp/dlp.log"
#define MAX_FILE_SIZE 5 * 1024 * 1024 // 5MB
#define MAX_FILES 5 // keep 5 rotated files

// Initialize logger (no args needed)
void InitLogger();

// Logging functions
void LogInfo(const std::string& msg);
void LogError(const std::string& msg);
void LogDebug(const std::string& msg);

#endif // DLP_LOGGER_H
