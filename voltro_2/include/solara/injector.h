#pragma once

#ifndef SOLARA_INJECTOR_H
#define SOLARA_INJECTOR_H

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

namespace Solara {
namespace Injector {




inline bool EnableDebugPrivilege() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    LUID luid;
    if (!LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &luid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    DWORD err = GetLastError();
    CloseHandle(hToken);
    return ok && (err != ERROR_NOT_ALL_ASSIGNED);
}




struct TargetProcessInfo {
    DWORD pid = 0;
    std::wstring processName;
    bool is64Bit = true;
};

class ProcessScanner {
public:
    static std::vector<TargetProcessInfo> findRobloxTargets() {
        std::vector<TargetProcessInfo> results;
        EnableDebugPrivilege();

        
        HWND hRbxWnd = FindWindowW(L"WINDOWSCLIENT", nullptr);
        if (!hRbxWnd) hRbxWnd = FindWindowW(nullptr, L"Roblox");
        if (hRbxWnd) {
            DWORD wndPid = 0;
            GetWindowThreadProcessId(hRbxWnd, &wndPid);
            if (wndPid != 0) {
                TargetProcessInfo info;
                info.pid = wndPid;
                info.processName = L"RobloxPlayerBeta.exe";
                info.is64Bit = true;
                results.push_back(info);
                return results;
            }
        }

        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return results;
        }

        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &entry)) {
            do {
                std::wstring exeName = entry.szExeFile;
                std::wstring lowerName = exeName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

                
                if (lowerName.find(L"crashhandler") != std::wstring::npos ||
                    lowerName.find(L"launcher") != std::wstring::npos ||
                    lowerName.find(L"installer") != std::wstring::npos ||
                    lowerName.find(L"studio") != std::wstring::npos) {
                    continue;
                }

                
                bool isRoblox = (lowerName == L"robloxplayerbeta.exe") ||
                                (lowerName == L"robloxplayer.exe") ||
                                (lowerName == L"windows10universal.exe") ||
                                (lowerName.find(L"roblox") != std::wstring::npos);

                if (isRoblox) {
                    TargetProcessInfo info;
                    info.pid = entry.th32ProcessID;
                    info.processName = entry.szExeFile;
                    info.is64Bit = true;

                    
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
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return results;
    }
};




inline bool IsModuleLoaded(DWORD pid, const std::string& moduleName) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return false;

    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    bool found = false;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        DWORD count = cbNeeded / sizeof(HMODULE);
        for (DWORD i = 0; i < count; ++i) {
            char szModName[MAX_PATH] = { 0 };
            if (GetModuleBaseNameA(hProcess, hMods[i], szModName, sizeof(szModName))) {
                std::string current(szModName);
                std::string target(moduleName);
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::transform(target.begin(), target.end(), target.begin(), ::tolower);
                if (current == target || current.find(target) != std::string::npos) {
                    found = true;
                    break;
                }
            }
        }
    }

    CloseHandle(hProcess);
    return found;
}





inline bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError) {
    EnableDebugPrivilege();

    
    DWORD fileAttr = GetFileAttributesA(dllPath.c_str());
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        outError = "DLL file not found: " + dllPath;
        return false;
    }

    
    char fullPath[MAX_PATH] = { 0 };
    if (!GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullPath, nullptr)) {
        outError = "Could not resolve full path for DLL: " + dllPath;
        return false;
    }

    
    std::string modFileName = fullPath;
    size_t lastSlash = modFileName.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        modFileName = modFileName.substr(lastSlash + 1);
    }
    if (IsModuleLoaded(pid, modFileName)) {
        outError.clear();
        return true;
    }

    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE,
        pid
    );

    if (!hProcess) {
        
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    if (!hProcess) {
        outError = "Failed to open target Roblox process (PID: " + std::to_string(pid) + "). Error: " + std::to_string(GetLastError());
        return false;
    }

    
    size_t pathLen = strlen(fullPath) + 1;
    LPVOID remoteMemory = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMemory) {
        CloseHandle(hProcess);
        outError = "Failed to allocate memory in target process. Error: " + std::to_string(GetLastError());
        return false;
    }

    
    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(hProcess, remoteMemory, fullPath, pathLen, &bytesWritten) || bytesWritten != pathLen) {
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        outError = "Failed to write DLL path into target process. Error: " + std::to_string(GetLastError());
        return false;
    }

    
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        outError = "Failed to locate kernel32.dll in host.";
        return false;
    }

    FARPROC loadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibraryAddr) {
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        outError = "Failed to locate LoadLibraryA.";
        return false;
    }

    
    HANDLE hRemoteThread = CreateRemoteThread(
        hProcess,
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
        remoteMemory,
        0,
        nullptr
    );

    if (!hRemoteThread) {
        
        typedef LONG (WINAPI* pfnNtCreateThreadEx)(
            PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, LPVOID ObjectAttributes,
            HANDLE ProcessHandle, LPTHREAD_START_ROUTINE StartRoutine, LPVOID Argument,
            ULONG CreateFlags, ULONG_PTR ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize,
            LPVOID AttributeList
        );
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            auto fnNtCreateThreadEx = reinterpret_cast<pfnNtCreateThreadEx>(GetProcAddress(hNtdll, "NtCreateThreadEx"));
            if (fnNtCreateThreadEx) {
                fnNtCreateThreadEx(&hRemoteThread, THREAD_ALL_ACCESS, nullptr, hProcess,
                                   reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
                                   remoteMemory, 0, 0, 0, 0, nullptr);
            }
        }
    }

    if (!hRemoteThread) {
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        outError = "CreateRemoteThread failed (Error: " + std::to_string(GetLastError()) + "). Target protected.";
        return false;
    }

    
    WaitForSingleObject(hRemoteThread, 3000);
    CloseHandle(hRemoteThread);

    
    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    outError.clear();
    return true;
}

} 
} 

#endif 
