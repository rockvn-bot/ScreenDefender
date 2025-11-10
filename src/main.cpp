#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "overlay.h"
#include "watermark.h"

extern bool enableOverlay;
extern bool enableTextWatermark;
extern bool enableImageWatermark;
extern std::string imageSource;
extern std::string textSource;
extern std::atomic<bool> overlayThreadRunning;

void PrintUsage() {
    std::cout << "Usage:\n"
        << "  --text <text>                 Use text watermark\n"
        << "  --image-path <path>           Use image file\n"
        << "  --base64-file <path>          Use base64 encoded file\n"
        << "  --overlay                     Disable display sharing (can be combined)\n"
        << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        PrintUsage();
        return 0;
    }

    bool hasPrimaryArg = false;  // To ensure only one primary argument is used

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--overlay") {
            enableOverlay = true;
        }
        else if (arg == "--text" && i + 1 < argc) {
            if (hasPrimaryArg) {
                std::cerr << "Error: Only one of --text, --image-path, or --base64-file can be used at a time.\n";
                return 1;
            }
            hasPrimaryArg = true;
            textSource = argv[++i];
            enableTextWatermark = true;
            enableImageWatermark = false;
        }
        else if (arg == "--image-path" && i + 1 < argc) {
            if (hasPrimaryArg) {
                std::cerr << "Error: Only one of --text, --image-path, or --base64-file can be used at a time.\n";
                return 1;
            }
            hasPrimaryArg = true;
            imageSource = argv[++i];
            enableImageWatermark = true;
            enableTextWatermark = false;
        }
        else if (arg == "--base64-file" && i + 1 < argc) {
            if (hasPrimaryArg) {
                std::cerr << "Error: Only one of --text, --image-path, or --base64-file can be used at a time.\n";
                return 1;
            }
            hasPrimaryArg = true;
            std::string filePath = argv[++i];
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                imageSource = buffer.str();
                // Remove whitespace/newlines
                imageSource.erase(
                    std::remove_if(imageSource.begin(), imageSource.end(),
                        [](unsigned char c) { return std::isspace(c); }),
                    imageSource.end()
                );
                enableImageWatermark = true;
                enableTextWatermark = false;
            }
            else {
                std::cerr << "Error: Unable to open base64 file: " << filePath << "\n";
                return 1;
            }
        }
        else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    if (!hasPrimaryArg && !enableOverlay) {
        std::cerr << "Error: You must specify one of --text, --image-path, or --base64-file (unless only --overlay is used).\n";
        return 1;
    }

    overlayThreadRunning = true;
    std::thread overlayThread(OverlayThreadProc);

    std::cout << "Watermark overlay running. Press Ctrl+C to exit.\n";

    if (overlayThread.joinable())
        overlayThread.join();

    return 0;
}
