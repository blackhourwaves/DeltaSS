#include "scanner.h"

#include <tlhelp32.h>

namespace deltass {

    std::wstring ProcessNameFromPid(DWORD pid) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            return L"?";
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        std::wstring result = L"?";
        if (Process32FirstW(snap, &entry)) {
            do {
                if (entry.th32ProcessID == pid) {
                    result = entry.szExeFile;
                    break;
                }
            } while (Process32NextW(snap, &entry));
        }

        CloseHandle(snap);
        return result;
    }

    namespace {

        BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lp) {
            auto* out = reinterpret_cast<std::vector<HiddenWindow>*>(lp);

            if (!IsWindowVisible(hwnd)) {
                return TRUE;
            }

            DWORD affinity = 0;
            if (!GetWindowDisplayAffinity(hwnd, &affinity)) {
                return TRUE;
            }

            if (affinity != WDA_EXCLUDEFROMCAPTURE) {
                return TRUE;
            }

            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);

            wchar_t title[512] = {};
            GetWindowTextW(hwnd, title, 512);

            out->push_back({ hwnd, pid, ProcessNameFromPid(pid), title });
            return TRUE;
        }

    }

    std::vector<HiddenWindow> ScanHiddenWindows() {
        std::vector<HiddenWindow> result;
        EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&result));
        return result;
    }

}