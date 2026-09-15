#pragma once

// ============================================================
//  VoltroInjector.h  --  Phase 4 Injection Engine
//  Extends / augments the base Voltro::InjectorEngine defined
//  in src/injector.h with manual-mapping, process-hollowing,
//  pattern-scanning, and IPC bridge facilities.
// ============================================================

#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <psapi.h>

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdint>

// Pull in the existing base framework
#include "../injector.h"

namespace Voltro {
namespace Injector {

// ----------------------------------------------------------
//  Enumerations
// ----------------------------------------------------------

enum class InjectionMethod {
    ManualMap,       // Reflective / manual PE mapping (stealth)
    ProcessHollow,   // Process hollowing via NtUnmapViewOfSection
    LoadLibrary      // Classic LoadLibraryA (delegates to InjectorEngine)
};

enum class InjectionStatus {
    Success,
    ProcessNotFound,
    AccessDenied,
    DllNotFound,
    MappingFailed,
    HookFailed
};

// ----------------------------------------------------------
//  Data Structures
// ----------------------------------------------------------

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

// ----------------------------------------------------------
//  NtUnmapViewOfSection typedef (resolved at runtime from ntdll)
// ----------------------------------------------------------
using NtUnmapViewOfSection_t = NTSTATUS(WINAPI*)(HANDLE, PVOID);

// ----------------------------------------------------------
//  VoltroInjector -- main injection engine (singleton)
// ----------------------------------------------------------

class VoltroInjector {
public:
    static VoltroInjector& instance();

    VoltroInjector(const VoltroInjector&)            = delete;
    VoltroInjector& operator=(const VoltroInjector&) = delete;

    // -- Process discovery ------------------------------------

    /// Enumerate all running Roblox player processes.
    /// Cross-references CreateToolhelp32Snapshot with EnumWindows
    /// to attach window titles; queries IsWow64Process for arch.
    std::vector<ProcessInfo> findRobloxProcesses();

    // -- Injection entry point --------------------------------

    /// Inject dllPath into the process identified by pid.
    /// Defaults to ManualMap; pass LoadLibrary to use the base engine.
    InjectionResult inject(DWORD              pid,
                           const std::string& dllPath,
                           InjectionMethod    method = InjectionMethod::ManualMap);

    // -- Attachment state -------------------------------------

    bool  isAttached()  const;
    DWORD attachedPid() const;
    bool  detach();

private:
    VoltroInjector()  = default;
    ~VoltroInjector() = default;

    // -- Core injection strategies ----------------------------

    /// Reflective manual-map: maps the PE into the target process
    /// without touching the loader or creating a module list entry.
    InjectionResult manualMap(HANDLE                   hProcess,
                              const std::vector<BYTE>& dllData);

    /// Process-hollowing: spawns svchost.exe suspended, unmaps its
    /// image, replaces with our PE, then resumes execution.
    InjectionResult processHollow(DWORD                    pid,
                                  const std::vector<BYTE>& dllData);

    // -- PE helper routines -----------------------------------

    /// Walk the IAT of the mapped image and resolve each import by
    /// locating the module in the target process and reading its
    /// export directory via ReadProcessMemory.
    bool resolveImports(HANDLE                    hProcess,
                        BYTE*                     mappedBase,
                        const IMAGE_NT_HEADERS64* nt);

    /// Apply base-relocation fixups for the delta between the
    /// preferred load address and the actual allocation address.
    bool fixRelocations(BYTE*                     mappedBase,
                        const IMAGE_NT_HEADERS64* nt,
                        uintptr_t                 delta);

    /// Locate an exported function RVA from a remote module by
    /// reading its export directory out of the target address space.
    DWORD findExportedFunction(HANDLE      hProcess,
                               HMODULE     hMod,
                               const char* funcName);

    /// Read a DLL file from disk into a byte vector.
    std::vector<BYTE> loadDllFromDisk(const std::string& path);

    // -- State ------------------------------------------------
    DWORD m_attachedPid = 0;
    bool  m_attached    = false;
};

// ----------------------------------------------------------
//  PatternScanner
//  Pattern format: "AA BB ? DD EE"  ('?' == wildcard byte)
// ----------------------------------------------------------

class PatternScanner {
public:
    /// Scan a memory region in a remote process for the first
    /// occurrence of pattern.  Returns 0 on failure / not found.
    static uintptr_t scan(HANDLE             hProcess,
                          DWORD_PTR          start,
                          SIZE_T             size,
                          const std::string& pattern);

    /// Same as scan() but collects every matching address.
    static std::vector<uintptr_t> scanAll(HANDLE             hProcess,
                                          DWORD_PTR          start,
                                          SIZE_T             size,
                                          const std::string& pattern);

    /// Compare raw bytes against a pattern string inline.
    static bool comparePattern(const BYTE*        data,
                               const std::string& pattern);

private:
    /// Parse a hex-pattern string into parallel byte/mask vectors.
    /// mask[i] == 0xFF: exact match required; 0x00: wildcard.
    static void parsePattern(const std::string& pattern,
                             std::vector<BYTE>& bytes,
                             std::vector<BYTE>& mask);
};

// ----------------------------------------------------------
//  IpcBridge -- Named-Pipe server/client for script execution
//
//  Pipe name  : \\.\pipe\VoltroIPC
//  Wire format: newline-delimited JSON objects
//    UI -> DLL  : {"type":"EXECUTE_SCRIPT","code":"..."}
//               : {"type":"STOP"}
//    DLL -> UI  : {"type":"OUTPUT","text":"...","level":"print|warn|error"}
// ----------------------------------------------------------

class IpcBridge {
public:
    static IpcBridge& instance();

    IpcBridge(const IpcBridge&)            = delete;
    IpcBridge& operator=(const IpcBridge&) = delete;

    // -- Server side (UI process owns the pipe) ---------------

    /// Create the named pipe and begin accepting one client on a
    /// background thread.  Returns false if the pipe already exists
    /// or cannot be created.
    bool startServer(const std::wstring& pipeName = L"\\\\.\\pipe\\VoltroIPC");

    // -- Client side (injected DLL or test harness) -----------

    /// Connect to an existing VoltroIPC server pipe.
    bool connect(const std::wstring& pipeName = L"\\\\.\\pipe\\VoltroIPC");

    // -- Messaging --------------------------------------------

    /// Serialize and transmit a Lua script to the injected DLL.
    bool sendScript(const std::string& code);

    /// Tell the injected DLL to halt the currently running script.
    bool sendStop();

    // -- State ------------------------------------------------
    bool isConnected() const;
    void disconnect();

    // -- Callbacks (assign before startServer / connect) ------

    /// Fires when a line of script output is received from the DLL.
    std::function<void(std::string /*text*/, std::string /*level*/)> onOutput;

    /// Fires once when the remote end connects to our pipe.
    std::function<void()> onConnected;

    /// Fires when the remote end disconnects / pipe is broken.
    std::function<void()> onDisconnected;

private:
    IpcBridge()  = default;
    ~IpcBridge();

    void serverLoop(std::wstring pipeName);  // background thread
    void clientReadLoop();                   // background thread

    bool        writeLine(const std::string& json);
    std::string readLine();
    void        dispatchMessage(const std::string& json);

    std::thread       m_serverThread;
    std::thread       m_readThread;
    HANDLE            m_pipe      = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_connected { false };
    std::atomic<bool> m_stopping  { false };
};

} // namespace Injector
} // namespace Voltro
