#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <thread>
#include <atomic>

namespace Thakur {
namespace Injector {

enum class InjectionMethod {
    ManualMap,       
    ProcessHollow,   
    LoadLibrary      
};

enum class InjectionStatus {
    Success,
    ProcessNotFound,
    AccessDenied,
    DllNotFound,
    MappingFailed,
    HookFailed
};

struct InjectionResult {
    InjectionStatus status      = InjectionStatus::MappingFailed;
    DWORD           targetPid   = 0;
    std::string     message;
    uintptr_t       baseAddress = 0;
};

struct ProcessInfo {
    DWORD         pid         = 0;
    std::wstring  name;
    std::wstring  windowTitle;
    bool          is64bit     = false;
};

using NtUnmapViewOfSection_t = NTSTATUS(WINAPI*)(HANDLE, PVOID);

class ThakurInjector {
public:
    static ThakurInjector& instance();

    ThakurInjector(const ThakurInjector&)            = delete;
    ThakurInjector& operator=(const ThakurInjector&) = delete;

    

    
    
    
    std::vector<ProcessInfo> findRobloxProcesses();

    

    
    
    InjectionResult inject(DWORD              pid,
                           const std::string& dllPath,
                           InjectionMethod    method = InjectionMethod::ManualMap);

    

    bool  isAttached()  const;
    DWORD attachedPid() const;
    bool  detach();

private:
    ThakurInjector()  = default;
    ~ThakurInjector() = default;

    

    
    
    InjectionResult manualMap(HANDLE                   hProcess,
                              const std::vector<BYTE>& dllData);

    
    
    InjectionResult processHollow(DWORD                    pid,
                                  const std::vector<BYTE>& dllData);

    

    
    
    
    bool resolveImports(HANDLE                    hProcess,
                        BYTE*                     mappedBase,
                        const IMAGE_NT_HEADERS64* nt);

    
    
    bool fixRelocations(BYTE*                     mappedBase,
                        const IMAGE_NT_HEADERS64* nt,
                        uintptr_t                 delta);

    
    
    DWORD findExportedFunction(HANDLE      hProcess,
                               HMODULE     hMod,
                               const char* funcName);

    
    std::vector<BYTE> loadDllFromDisk(const std::string& path);

    
    DWORD m_attachedPid = 0;
    bool  m_attached    = false;
};

class PatternScanner {
public:
    
    
    static uintptr_t scan(HANDLE             hProcess,
                          DWORD_PTR          start,
                          SIZE_T             size,
                          const std::string& pattern);

    
    static std::vector<uintptr_t> scanAll(HANDLE             hProcess,
                                          DWORD_PTR          start,
                                          SIZE_T             size,
                                          const std::string& pattern);

    
    static bool comparePattern(const BYTE*        data,
                               const std::string& pattern);

private:
    
    
    static void parsePattern(const std::string& pattern,
                             std::vector<BYTE>& bytes,
                             std::vector<BYTE>& mask);
};

class IpcBridge {
public:
    static IpcBridge& instance();

    IpcBridge(const IpcBridge&)            = delete;
    IpcBridge& operator=(const IpcBridge&) = delete;

    

    
    
    
    bool startServer(const std::wstring& pipeName = L"\\\\.\\pipe\\ThakurIPC");

    

    
    bool connect(const std::wstring& pipeName = L"\\\\.\\pipe\\ThakurIPC");

    

    
    bool sendScript(const std::string& code);

    
    bool sendStop();

    
    bool isConnected() const;
    void disconnect();

    

    
    std::function<void(std::string , std::string )> onOutput;

    
    std::function<void()> onConnected;

    
    std::function<void()> onDisconnected;

private:
    IpcBridge()  = default;
    ~IpcBridge();

    void serverLoop(std::wstring pipeName);  
    void clientReadLoop();                   

    bool        writeLine(const std::string& json);
    std::string readLine();
    void        dispatchMessage(const std::string& json);

    std::thread       m_serverThread;
    std::thread       m_readThread;
    HANDLE            m_pipe      = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_connected { false };
    std::atomic<bool> m_stopping  { false };
};

} 
} 
