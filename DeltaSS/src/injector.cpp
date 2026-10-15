#include "injector.h"
#include "payload_blob.h"

namespace deltass {

    namespace {

        std::wstring TempPathForPayload() {
            wchar_t tempDir[MAX_PATH] = {};
            GetTempPathW(MAX_PATH, tempDir);

            wchar_t fileName[64] = {};
            swprintf_s(fileName, L"deltastreamproof_%lu.dll", GetCurrentProcessId());

            return std::wstring(tempDir) + fileName;
        }

    }

    bool ExtractPayloadToTemp(std::wstring& outPath) {
        const std::wstring path = TempPathForPayload();

        HANDLE file = CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
            nullptr
        );

        if (file == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD written = 0;
        WriteFile(
            file,
            payload::kDllBytes,
            static_cast<DWORD>(payload::kDllSize),
            &written,
            nullptr
        );
        CloseHandle(file);

        if (written != payload::kDllSize) {
            return false;
        }

        outPath = path;
        return true;
    }

    void CleanupPayload(const std::wstring& path) {
        if (path.empty()) return;
        DeleteFileW(path.c_str());
    }

    bool InjectPayloadIntoProcess(DWORD pid, const std::wstring& dllPath) {
        HANDLE process = OpenProcess(
            PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE |
            PROCESS_VM_READ,
            FALSE,
            pid
        );

        if (!process) {
            return false;
        }

        const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
        void* remote = VirtualAllocEx(
            process,
            nullptr,
            bytes,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if (!remote) {
            CloseHandle(process);
            return false;
        }

        if (!WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr)) {
            VirtualFreeEx(process, remote, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }

        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(kernel32, "LoadLibraryW")
            );

        HANDLE thread = CreateRemoteThread(
            process,
            nullptr,
            0,
            loadLibrary,
            remote,
            0,
            nullptr
        );

        bool ok = false;
        if (thread) {
            WaitForSingleObject(thread, 5000);
            CloseHandle(thread);
            ok = true;
        }

        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return ok;
    }

    void ScheduleSelfDelete() {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        wchar_t tempDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tempDir);

        wchar_t batPath[MAX_PATH] = {};
        swprintf_s(batPath, L"%sdeltass_cleanup_%lu.bat", tempDir, GetCurrentProcessId());

        HANDLE bat = CreateFileW(
            batPath,
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (bat == INVALID_HANDLE_VALUE) {
            return;
        }

        char script[1024] = {};
        sprintf_s(
            script,
            "@echo off\r\n"
            ":loop\r\n"
            "del \"%ls\" >nul 2>&1\r\n"
            "if exist \"%ls\" (\r\n"
            "  timeout /t 1 /nobreak >nul\r\n"
            "  goto loop\r\n"
            ")\r\n"
            "del \"%ls\" >nul 2>&1\r\n",
            exePath, exePath, batPath
        );

        DWORD written = 0;
        WriteFile(bat, script, static_cast<DWORD>(strlen(script)), &written, nullptr);
        CloseHandle(bat);

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi{};
        CreateProcessW(
            nullptr,
            batPath,
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (pi.hProcess) CloseHandle(pi.hProcess);
        if (pi.hThread) CloseHandle(pi.hThread);
    }

}