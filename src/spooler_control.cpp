#include "spooler_control.h"
#include <iostream>
#include <windows.h>
#include <cstdlib>  // system()

static void runCommand(const std::string& command)
{
    std::cout << "> " << command << std::endl;
    int result = system(command.c_str());
    if (result != 0)
    {
        std::cerr << "Command failed with code: " << result << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}

bool CheckAdmin()
{
    BOOL isAdmin = FALSE;
    HANDLE token = nullptr;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        TOKEN_ELEVATION elevation;
        DWORD size;

        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return isAdmin;
}

void enableSpooler()
{
    std::cout << "\n=== Enabling Print Spooler ===\n";

    runCommand("sc sdset spooler \"D:(A;;CCLCSWLOCRRC;;;AU)"
        "(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)"
        "(A;;CCLCSWRPWPDTLOCRRC;;;SY)"
        "S:(AU;FA;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;WD)\"");

    runCommand("sc config spooler start= auto");
    runCommand("sc start spooler");

    std::cout << "Print Spooler ENABLED successfully.\n";
}

void disableSpooler()
{
    std::cout << "\n=== Disabling Print Spooler ===\n";

    runCommand("sc stop spooler");
    runCommand("sc config spooler start= disabled");
    runCommand("sc sdset spooler \"D:(D;;RPWPCC;;;AU)"
        "(A;;CCLCSWLOCRRC;;;AU)"
        "(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)"
        "(A;;CCLCSWRPWPDTLOCRRC;;;SY)"
        "S:(AU;FA;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;WD)\"");

    std::cout << "Print Spooler DISABLED successfully.\n";
}
