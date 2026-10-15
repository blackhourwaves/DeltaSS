#include <windows.h>

namespace {

    BOOL CALLBACK ClearAffinity(HWND hwnd, LPARAM) {
        DWORD affinity = 0;
        if (GetWindowDisplayAffinity(hwnd, &affinity)) {
            if (affinity == WDA_EXCLUDEFROMCAPTURE) {
                SetWindowDisplayAffinity(hwnd, WDA_NONE);
            }
        }
        return TRUE;
    }

}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        EnumWindows(ClearAffinity, 0);
    }
    return TRUE;
}