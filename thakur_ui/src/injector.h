#pragma once
#include <windows.h>
#include <string>
#include <TlHelp32.h>

namespace Thakur {

enum class InjectionStatus {
    SUCCESS,
    PROCESS_NOT_FOUND,
    OPEN_PROCESS_FAILED,
    MEMORY_ALLOCATION_FAILED,
    WRITE_MEMORY_FAILED,
    REMOTE_THREAD_FAILED,
    INTEGRITY_CHECK_FAILED
};

struct InjectionResult {
    InjectionStatus status;
    std::string message;
    DWORD targetPid = 0;
};

class InjectorEngine {
public:
    static std::string getDllAbsolutePath(const std::string& dllName) {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string path(buffer);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return path.substr(0, lastSlash + 1) + dllName;
        }
        return dllName;
    }

    
    static InjectionResult injectDll(DWORD pid, const std::string& dllPath) {
        InjectionResult res;
        res.targetPid = pid;

        if (pid == 0) {
            res.status = InjectionStatus::PROCESS_NOT_FOUND;
            res.message = "Target process not running or PID is invalid.";
            return res;
        }

        std::string targetDllPath = dllPath;
        if (targetDllPath.find(":\\") == std::string::npos && targetDllPath.find(":/") == std::string::npos) {
            targetDllPath = getDllAbsolutePath(dllPath);
        }

        
        HANDLE hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD,
            FALSE,
            pid
        );

        if (!hProcess) {
            res.status = InjectionStatus::OPEN_PROCESS_FAILED;
            res.message = "Failed to open target process handle (Error: " + std::to_string(GetLastError()) + "). Elevated permissions required.";
            return res;
        }

        
        LPVOID remoteMemory = VirtualAllocEx(hProcess, NULL, targetDllPath.length() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remoteMemory) {
            CloseHandle(hProcess);
            res.status = InjectionStatus::MEMORY_ALLOCATION_FAILED;
            res.message = "VirtualAllocEx failed in remote process space.";
            return res;
        }

        
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, remoteMemory, targetDllPath.c_str(), targetDllPath.length() + 1, &bytesWritten) || bytesWritten == 0) {
            VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            res.status = InjectionStatus::WRITE_MEMORY_FAILED;
            res.message = "WriteProcessMemory failed to stage payload buffer.";
            return res;
        }

        
        LPVOID loadLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
        if (!loadLibraryAddr) {
            VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            res.status = InjectionStatus::REMOTE_THREAD_FAILED;
            res.message = "Could not resolve LoadLibraryA address.";
            return res;
        }

        
        
        bool threadHijacked = false;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnap != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te;
            te.dwSize = sizeof(te);
            if (Thread32First(hSnap, &te)) {
                do {
                    if (te.th32OwnerProcessID == pid) {
                        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, te.th32ThreadID);
                        if (hThread) {
                            DWORD suspendCount = SuspendThread(hThread);
                            if (suspendCount != (DWORD)-1) {
                                CONTEXT ctx = { 0 };
                                ctx.ContextFlags = CONTEXT_FULL;
                                if (GetThreadContext(hThread, &ctx)) {
                                    
                                    
                                    
                                    std::vector<BYTE> stub;
                                    stub.reserve(256);

                                    
                                    stub.push_back(0x9C);
                                    
                                    BYTE gprPush1[] = { 0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57 };
                                    stub.insert(stub.end(), std::begin(gprPush1), std::end(gprPush1));
                                    
                                    BYTE gprPush2[] = { 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57 };
                                    stub.insert(stub.end(), std::begin(gprPush2), std::end(gprPush2));

                                    
                                    BYTE xmmSaveSub[] = { 0x48, 0x83, 0xEC, 0x60 };
                                    stub.insert(stub.end(), std::begin(xmmSaveSub), std::end(xmmSaveSub));
                                    BYTE xmmSave[] = {
                                        0xF3, 0x0F, 0x7F, 0x04, 0x24,                   
                                        0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x10,             
                                        0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x20,             
                                        0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x30,             
                                        0xF3, 0x0F, 0x7F, 0x64, 0x24, 0x40,             
                                        0xF3, 0x0F, 0x7F, 0x6C, 0x24, 0x50              
                                    };
                                    stub.insert(stub.end(), std::begin(xmmSave), std::end(xmmSave));

                                    
                                    
                                    
                                    
                                    
                                    BYTE alignStack[] = {
                                        0x48, 0x89, 0xE0,
                                        0x48, 0x83, 0xE4, 0xF0,
                                        0x48, 0x83, 0xEC, 0x20,
                                        0x50
                                    };
                                    stub.insert(stub.end(), std::begin(alignStack), std::end(alignStack));

                                    
                                    stub.push_back(0x48); stub.push_back(0xB9);
                                    uint64_t targetPathAddr = reinterpret_cast<uint64_t>(remoteMemory);
                                    BYTE* pTarget = reinterpret_cast<BYTE*>(&targetPathAddr);
                                    stub.insert(stub.end(), pTarget, pTarget + 8);

                                    
                                    stub.push_back(0x48); stub.push_back(0xB8);
                                    uint64_t loadLibAddrVal = reinterpret_cast<uint64_t>(loadLibraryAddr);
                                    BYTE* pLoad = reinterpret_cast<BYTE*>(&loadLibAddrVal);
                                    stub.insert(stub.end(), pLoad, pLoad + 8);

                                    
                                    stub.push_back(0xFF); stub.push_back(0xD0);

                                    
                                    stub.push_back(0x5C);

                                    
                                    BYTE xmmRestore[] = {
                                        0xF3, 0x0F, 0x6F, 0x6C, 0x24, 0x50,             
                                        0xF3, 0x0F, 0x6F, 0x64, 0x24, 0x40,             
                                        0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x30,             
                                        0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x20,             
                                        0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x10,             
                                        0xF3, 0x0F, 0x6F, 0x04, 0x24                    
                                    };
                                    stub.insert(stub.end(), std::begin(xmmRestore), std::end(xmmRestore));

                                    
                                    BYTE xmmRestoreAdd[] = { 0x48, 0x83, 0xC4, 0x60 };
                                    stub.insert(stub.end(), std::begin(xmmRestoreAdd), std::end(xmmRestoreAdd));

                                    
                                    BYTE gprPop2[] = { 0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58 };
                                    stub.insert(stub.end(), std::begin(gprPop2), std::end(gprPop2));
                                    
                                    BYTE gprPop1[] = { 0x5F, 0x5E, 0x5D, 0x5B, 0x5A, 0x59, 0x58 };
                                    stub.insert(stub.end(), std::begin(gprPop1), std::end(gprPop1));
                                    
                                    stub.push_back(0x9D);

                                    
                                    stub.push_back(0x48); stub.push_back(0xB8);
                                    uint64_t oldRip = static_cast<uint64_t>(ctx.Rip);
                                    BYTE* pOldRip = reinterpret_cast<BYTE*>(&oldRip);
                                    stub.insert(stub.end(), pOldRip, pOldRip + 8);

                                    
                                    stub.push_back(0xFF); stub.push_back(0xE0);

                                    LPVOID stubMem = VirtualAllocEx(hProcess, NULL, stub.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
                                    if (stubMem) {
                                        SIZE_T stubWritten = 0;
                                        if (WriteProcessMemory(hProcess, stubMem, stub.data(), stub.size(), &stubWritten) && stubWritten == stub.size()) {
                                            ctx.Rip = reinterpret_cast<DWORD64>(stubMem);
                                            if (SetThreadContext(hThread, &ctx)) {
                                                threadHijacked = true;
                                            }
                                        }
                                    }
                                }
                                ResumeThread(hThread);
                            }
                            CloseHandle(hThread);
                            if (threadHijacked) break;
                        }
                    }
                } while (Thread32Next(hSnap, &te));
            }
            CloseHandle(hSnap);
        }

        if (threadHijacked) {
            CloseHandle(hProcess);
            res.status = InjectionStatus::SUCCESS;
            res.message = "Stealth thread-hijack injection successful into target process (PID: " + std::to_string(pid) + ").";
            return res;
        }

        
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr), remoteMemory, 0, NULL);
        if (!hThread) {
            VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            res.status = InjectionStatus::REMOTE_THREAD_FAILED;
            res.message = "CreateRemoteThread failed (Protected by Anti-Tamper / Page Guard).";
            return res;
        }

        
        WaitForSingleObject(hThread, 4000);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);

        res.status = InjectionStatus::SUCCESS;
        res.message = "Injection staged successfully into target process (PID: " + std::to_string(pid) + ").";
        return res;
    }
};

} 
