#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "overlay.h"
#include "watermark.h"
#include "spooler_control.h"
#include "storage_blocker.h"
#include "clipboard_blocker.h"
#include "dlp_logger.h"

extern bool enableOverlay;
extern bool enableTextWatermark;
extern bool enableImageWatermark;
extern std::string imageSource;
extern std::string textSource;
extern std::atomic<bool> overlayThreadRunning;

void PrintUsage() {
    LogInfo(
        "Usage:\n"
        "  --text <text>                 Apply a text-based watermark\n"
        "  --image-path <path>           Apply a watermark using an image\n"
        "  --base64-file <path>          Apply a watermark using a Base64 file\n"
        "  --overlay                     Enable screen overlay (screen-sharing protection)\n"
        "\n"
        "  --enable-spooler              Start and unlock the Windows Print Spooler\n"
        "  --disable-spooler             Stop and restrict the Windows Print Spooler\n"
        "\n"
        "  --enable-storage              Enable blocked USB/WPD devices\n"
        "  --disable-storage             Disable USB storage & WPD devices and monitor changes\n"
        "\n"
        "  --block-clipboard             Block clipboard copy/paste operations\n"
        "  --unblock-clipboard           Disable clipboard blocking\n"
        "\n"
        "  --help, -h                    Show this help message\n"
    );

}

int main(int argc, char* argv[])
{
    InitLogger();
    LogInfo("DLP Client started.");

    if (argc <= 1) {
        LogInfo("No arguments provided.\n");
        PrintUsage();
        return 0;
    }

    bool hasPrimaryArg = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--overlay") {
            LogInfo( "Screen overlay enabled.");
            enableOverlay = true;
        }
        else if (arg == "--text" && i + 1 < argc) {
            if (hasPrimaryArg) {
                LogError( "Only one primary watermark source can be used at a time.");
                return 1;
            }
            hasPrimaryArg = true;
            textSource = argv[++i];
            enableTextWatermark = true;
            enableImageWatermark = false;
            LogInfo( "Text watermark set.");
        }
        else if (arg == "--image-path" && i + 1 < argc) {
            if (hasPrimaryArg) {
                LogError( "Only one primary watermark source can be used at a time.");
                return 1;
            }
            hasPrimaryArg = true;
            imageSource = argv[++i];
            enableImageWatermark = true;
            enableTextWatermark = false;
            LogInfo( "Image watermark loaded from path.");
        }
        else if (arg == "--base64-file" && i + 1 < argc) {
            if (hasPrimaryArg) {
                LogError( "Only one primary watermark source can be used at a time.");
                return 1;
            }
            hasPrimaryArg = true;

            std::string filePath = argv[++i];
            std::ifstream file(filePath);

            if (!file.is_open()) {
                LogError( "Could not open Base64 file: " + filePath);
                return 1;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            imageSource = buffer.str();

            imageSource.erase(
                std::remove_if(imageSource.begin(), imageSource.end(),
                    [](unsigned char c) { return std::isspace(c); }),
                imageSource.end()
            );

            enableImageWatermark = true;
            enableTextWatermark = false;

            LogInfo( "Base64 watermark file loaded.");
        }
        else if (arg == "--enable-spooler") {
            if (!CheckAdmin()) {
                LogError( "Administrator privileges required to enable spooler.");
                return 1;
            }
            LogInfo( "Enabling Windows Print Spooler...");
            enableSpooler();
            LogInfo( "[OK] Print Spooler enabled.");
            return 0;
        }
        else if (arg == "--disable-spooler") {
            if (!CheckAdmin()) {
                LogError( "Administrator privileges required to disable spooler.");
                return 1;
            }
            LogInfo( "Disabling Windows Print Spooler...");
            disableSpooler();
            LogInfo( "[OK] Print Spooler disabled.");
            return 0;
        }
        else if (arg == "--disable-storage") {
            if (!SB_IsRunningAsAdmin()) {
                LogError( "Administrator privileges required to disable storage.");
                return 1;
            }
            LogInfo( "Disabling USB/WPD devices and starting monitor...");
            SB_DisableAndMonitor();
            return 0;
        }
        else if (arg == "--enable-storage") {
            if (!SB_IsRunningAsAdmin()) {
                LogError( "Administrator privileges required to enable storage.");
                return 1;
            }
            LogInfo( "Re-enabling USB/WPD devices...");
            SB_EnableOnly();
            LogInfo( "[OK] USB/WPD devices re-enabled.");
            return 0;
        }
        else if (arg == "--block-clipboard") {
            LogInfo( "Clipboard blocking activated.");
            CB_StartBlocking();
            return 0;
        }
        else if (arg == "--unblock-clipboard") {
            LogInfo( "Clipboard blocking deactivated.");
            CB_StopBlocking();
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        }
        else {
            LogError( "Unknown option: " + arg);
            PrintUsage();
            return 1;
        }
    }

    if (!hasPrimaryArg && !enableOverlay) {
        LogError( "A watermark source is required unless using --overlay only.");
        return 1;
    }

    LogInfo( "Starting screen watermark overlay...");

    overlayThreadRunning = true;
    std::thread overlayThread(OverlayThreadProc);

    LogInfo( "Overlay running. Press Ctrl+C to exit.");

    if (overlayThread.joinable())
        overlayThread.join();

    LogInfo( "Overlay stopped.");

    return 0;
}
