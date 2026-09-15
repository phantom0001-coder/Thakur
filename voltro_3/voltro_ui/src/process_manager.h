#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <functional>

namespace Voltro {

struct ProcessInfo {
    DWORD pid = 0;
    std::wstring processName;
    bool is64Bit = true;
};

class ProcessManager {
public:
    // Callback to check if a process has an actual visible top-level window
    static BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam) {
        DWORD targetPid = *reinterpret_cast<DWORD*>(lParam);
        DWORD procId = 0;
        GetWindowThreadProcessId(hWnd, &procId);
        if (procId == targetPid && IsWindowVisible(hWnd)) {
            RECT r;
            if (GetWindowRect(hWnd, &r)) {
                if ((r.right - r.left) > 60 && (r.bottom - r.top) > 60) {
                    *reinterpret_cast<DWORD*>(lParam) = 0; // Found active window
                    return FALSE; // Stop enumeration
                }
            }
        }
        return TRUE;
    }

    static bool hasActiveGuiWindow(DWORD pid) {
        if (pid == 0) return false;
        DWORD checkPid = pid;
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&checkPid));
        return (checkPid == 0);
    }

    // Scans running processes for target Roblox client with verified active GUI window
    static std::vector<ProcessInfo> findTargetProcesses(const std::wstring& targetName = L"RobloxPlayerBeta.exe") {
        std::vector<ProcessInfo> results;

        // 1. Check for active Roblox game window class directly
        HWND hRbxWnd = FindWindowW(L"WINDOWSCLIENT", nullptr);
        if (!hRbxWnd) {
            hRbxWnd = FindWindowW(nullptr, L"Roblox");
        }
        if (hRbxWnd && IsWindowVisible(hRbxWnd)) {
            DWORD wndPid = 0;
            GetWindowThreadProcessId(hRbxWnd, &wndPid);
            if (wndPid != 0) {
                ProcessInfo info;
                info.pid = wndPid;
                info.processName = L"RobloxPlayerBeta.exe";
                info.is64Bit = true;
                results.push_back(info);
                return results;
            }
        }

        // 2. Snapshot scan for process with verified visible window (never attach to headless zombies)
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return results;
        }

        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &entry)) {
            do {
                bool matches = (_wcsicmp(entry.szExeFile, targetName.c_str()) == 0) ||
                               (_wcsicmp(entry.szExeFile, L"RobloxPlayerBeta.exe") == 0) ||
                               (_wcsicmp(entry.szExeFile, L"RobloxPlayer.exe") == 0) ||
                               (_wcsicmp(entry.szExeFile, L"Windows10Universal.exe") == 0);
                if (matches) {
                    // Ignore headless/crashed zombie processes
                    if (hasActiveGuiWindow(entry.th32ProcessID)) {
                        ProcessInfo info;
                        info.pid = entry.th32ProcessID;
                        info.processName = entry.szExeFile;
                        
                        // Check bitness
                        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
                        if (hProc) {
                            BOOL isWow64 = FALSE;
                            if (IsWow64Process(hProc, &isWow64)) {
                                info.is64Bit = !isWow64;
                            }
                            CloseHandle(hProc);
                        }
                        results.push_back(info);
                    }
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return results;
    }

    // Verifies whether a given PID is still active
    static bool isProcessAlive(DWORD pid) {
        if (pid == 0) return false;
        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!hProc) return false;
        DWORD exitCode = 0;
        GetExitCodeProcess(hProc, &exitCode);
        CloseHandle(hProc);
        return (exitCode == STILL_ACTIVE);
    }
};

// -----------------------------------------------------------------------------
// RobloxWatchdog: Real-time Crash Supervisor & Auto-Recovery
// -----------------------------------------------------------------------------
class RobloxWatchdog {
public:
    static RobloxWatchdog& getInstance() {
        static RobloxWatchdog inst;
        return inst;
    }

    void startWatch(DWORD pid, std::function<void(DWORD exitCode, const std::string& diagnostic)> onExitOrCrash) {
        stopWatch();
        if (pid == 0) return;

        m_watchedPid = pid;
        m_running = true;

        m_watchThread = std::thread([this, pid, onExitOrCrash]() {
            HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!hProc) {
                m_running = false;
                m_watchedPid = 0;
                return;
            }

            WaitForSingleObject(hProc, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(hProc, &exitCode);
            CloseHandle(hProc);

            if (m_running && onExitOrCrash) {
                std::string diag;
                if (exitCode == 0xC0000005) {
                    diag = "EXCEPTION_ACCESS_VIOLATION (0xC0000005) - Roblox terminated due to memory violation or anti-tamper page trip.";
                } else if (exitCode == 0xC0000409) {
                    diag = "STATUS_STACK_BUFFER_OVERRUN (0xC0000409) - Roblox security guard triggered fatal termination.";
                } else if (exitCode == 0) {
                    diag = "Normal process exit (Exit Code: 0).";
                } else {
                    char hex[64];
                    sprintf_s(hex, "Exit Code: 0x%08X", exitCode);
                    diag = std::string(hex) + " - Roblox process terminated abnormally.";
                }
                onExitOrCrash(exitCode, diag);
            }
            m_running = false;
            m_watchedPid = 0;
        });
    }

    void stopWatch() {
        m_running = false;
        if (m_watchThread.joinable()) {
            m_watchThread.detach();
        }
        m_watchedPid = 0;
    }

    DWORD getWatchedPid() const {
        return m_watchedPid.load();
    }

    bool isWatching() const {
        return m_running.load();
    }

private:
    RobloxWatchdog() = default;
    ~RobloxWatchdog() { stopWatch(); }

    std::atomic<bool> m_running{ false };
    std::atomic<DWORD> m_watchedPid{ 0 };
    std::thread m_watchThread;
};

} // namespace Voltro
