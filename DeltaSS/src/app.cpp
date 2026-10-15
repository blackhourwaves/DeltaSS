#include "app.h"
#include "injector.h"
#include "scanner.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

namespace deltass {

    namespace {

        constexpr const char* kReset = "\x1b[0m";
        constexpr const char* kBold = "\x1b[1m";
        constexpr const char* kDim = "\x1b[2m";
        constexpr const char* kClear = "\x1b[2J\x1b[H";

        constexpr DWORD kScanIntervalMs = 800;
        constexpr int kScansPerCycle = 3;

        HANDLE g_console = nullptr;
        std::wstring g_payloadPath;

        void EnableAnsi() {
            g_console = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            GetConsoleMode(g_console, &mode);
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            mode |= ENABLE_PROCESSED_OUTPUT;
            SetConsoleMode(g_console, mode);
        }

        void SetUtf8() {
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
        }

        std::string WideToUtf8(const std::wstring& w) {
            if (w.empty()) return {};
            const int size = WideCharToMultiByte(
                CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                nullptr, 0, nullptr, nullptr
            );
            std::string out(size, 0);
            WideCharToMultiByte(
                CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                out.data(), size, nullptr, nullptr
            );
            return out;
        }

        void Clear() {
            std::cout << kClear;
        }

        void DrawHeader() {
            std::cout << kBold
                << "  ██████╗ ███████╗██╗  ████████╗ █████╗ \n"
                << "  ██╔══██╗██╔════╝██║  ╚══██╔══╝██╔══██╗\n"
                << "  ██║  ██║█████╗  ██║     ██║   ███████║\n"
                << "  ██║  ██║██╔══╝  ██║     ██║   ██╔══██║\n"
                << "  ██████╔╝███████╗███████╗██║   ██║  ██║\n"
                << "  ╚═════╝ ╚══════╝╚══════╝╚═╝   ╚═╝  ╚═╝\n"
                << kReset;
        }

        void DrawRule() {
            std::cout << "  " << std::string(72, '-') << "\n";
        }

        void DrawScanning(int current, int total) {
            Clear();
            DrawHeader();
            DrawRule();
            std::cout << "\n";
            std::cout << "  Escaneando... (" << current << "/" << total << ")\n";
        }

        void DrawNothingFound() {
            Clear();
            DrawHeader();
            DrawRule();
            std::cout << "\n";
            std::cout << "  Nada detectado.\n";
        }

        void DrawAlert(const std::vector<HiddenWindow>& windows) {
            Clear();
            DrawHeader();
            DrawRule();
            std::cout << "\n";

            std::cout << kBold
                << "  [!] " << windows.size() << " Janelas detectada(s)"
                << kReset << "\n\n";

            for (size_t i = 0; i < windows.size(); ++i) {
                const auto& w = windows[i];

                std::string title = w.title.empty() ? "(sem titulo)" : WideToUtf8(w.title);
                if (title.size() > 50) {
                    title = title.substr(0, 47) + "...";
                }

                std::string proc = WideToUtf8(w.process);

                std::cout << "  [" << (i + 1) << "] "
                    << kBold << proc << kReset
                    << " (PID " << w.pid << ")\n";

                std::cout << "      " << kDim << title << kReset << "\n\n";
            }

            DrawRule();
            std::cout << "\n";
        }

        void DrawMenu() {
            std::cout << "  [1] Remover todos    "
                << "[Q] Sair\n\n";
            std::cout << "  > " << std::flush;
        }

        void StripAll(const std::vector<HiddenWindow>& windows) {
            if (g_payloadPath.empty()) {
                std::cout << "  Erro: ao remover Stream Mode\n";
                Sleep(1200);
                return;
            }

            for (const auto& w : windows) {
                std::cout << "  Removendo no processo " << WideToUtf8(w.process)
                    << " (PID " << w.pid << ")...\n";

                if (InjectPayloadIntoProcess(w.pid, g_payloadPath)) {
                    std::cout << "  [ok] Streamproof Removido\n";
                }
                else {
                    std::cout << "  [erro] Falha na Remocao\n";
                }
            }

            Sleep(800);
        }

        void HandleDetection(std::vector<HiddenWindow>& windows) {
            while (true) {
                DrawAlert(windows);
                DrawMenu();

                std::string line;
                std::getline(std::cin, line);

                if (line.empty()) {
                    continue;
                }

                const char c = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(line[0]))
                    );

                if (c == 'q' || c == 's') {
                    ExitProcess(0);
                }
                if (c == '1' || c == 'r') {
                    StripAll(windows);
                    return;
                }
            }
        }

    }

    void RunApplication(HINSTANCE, int) {
        EnableAnsi();
        SetUtf8();

        SetConsoleTitleW(L"Delta SS");

        ExtractPayloadToTemp(g_payloadPath);

        std::vector<HiddenWindow> windows;

        for (int i = 1; i <= kScansPerCycle; ++i) {
            DrawScanning(i, kScansPerCycle);
            Sleep(kScanIntervalMs);

            windows = ScanHiddenWindows();
            if (!windows.empty()) {
                break;
            }
        }

        if (windows.empty()) {
            DrawNothingFound();
            Sleep(1500);
            CleanupPayload(g_payloadPath);
            ScheduleSelfDelete();
            return;
        }

        HandleDetection(windows);

        CleanupPayload(g_payloadPath);
        ScheduleSelfDelete();
    }
}