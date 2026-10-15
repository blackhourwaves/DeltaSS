#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace deltass {

    struct HiddenWindow {
        HWND hwnd;
        DWORD pid;
        std::wstring process;
        std::wstring title;
    };

    std::vector<HiddenWindow> ScanHiddenWindows();
    std::wstring ProcessNameFromPid(DWORD pid);

}