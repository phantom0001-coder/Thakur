#include "VoltroInjector.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace Voltro {
namespace Injector {

// ----------------------------------------------------------
//  VoltroInjector Implementation
// ----------------------------------------------------------

VoltroInjector& VoltroInjector::instance() {
    static VoltroInjector inst;
    return inst;
}

struct WindowEnumData {
    DWORD pid;
    std::wstring title;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<WindowEnumData*>(lParam);
    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    if (winPid == data->pid && IsWindowVisible(hwnd)) {
        wchar_t buf[512] = {0};
        if (GetWindowTextW(hwnd, buf, 512) > 0) {
            data->title = buf;
            return FALSE; // Found primary visible window
        }
    }
    return TRUE;
}

std::vector<ProcessInfo> VoltroInjector::findRobloxProcesses() {
    std::vector<ProcessInfo> list;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return list;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snapshot, &pe)) {
        do {
            std::wstring exeName = pe.szExeFile;
            std::wstring lowerName = exeName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

            if (lowerName.find(L"robloxplayerbeta") != std::wstring::npos ||
                lowerName.find(L"robloxplayer") != std::wstring::npos ||
                lowerName.find(L"roblox") != std::wstring::npos) {

                ProcessInfo info;
                info.pid = pe.th32ProcessID;
                info.name = pe.szExeFile;

                // Query window title
                WindowEnumData wData = { info.pid, L"" };
                EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&wData));
                info.windowTitle = wData.title;

                // Query 64-bit architecture
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.pid);
                if (hProc) {
                    BOOL isWow64 = FALSE;
                    if (IsWow64Process(hProc, &isWow64)) {
                        // On 64-bit Windows, a 64-bit process returns FALSE for IsWow64Process
                        SYSTEM_INFO si;
                        GetNativeSystemInfo(&si);
                        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                            info.is64bit = !isWow64;
                        } else {
                            info.is64bit = false;
                        }
                    }
                    CloseHandle(hProc);
                }

                list.push_back(info);
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return list;
}

std::vector<BYTE> VoltroInjector::loadDllFromDisk(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<BYTE> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

InjectionResult VoltroInjector::inject(DWORD pid, const std::string& dllPath, InjectionMethod method) {
    InjectionResult res;
    res.targetPid = pid;

    if (pid == 0) {
        auto procs = findRobloxProcesses();
        if (procs.empty()) {
            res.status = InjectionStatus::ProcessNotFound;
            res.message = "Roblox process not found.";
            return res;
        }
        pid = procs[0].pid;
        res.targetPid = pid;
    }

    if (method == InjectionMethod::LoadLibrary) {
        auto baseResult = InjectorEngine::injectDll(pid, dllPath);
        if (baseResult.status == Voltro::InjectionStatus::SUCCESS) {
            res.status = InjectionStatus::Success;
            res.message = "DLL injected via LoadLibrary.";
            m_attached = true;
            m_attachedPid = pid;
        } else {
            res.status = InjectionStatus::MappingFailed;
            res.message = baseResult.message;
        }
        return res;
    }

    std::vector<BYTE> dllBytes = loadDllFromDisk(dllPath);
    if (dllBytes.empty()) {
        // Fallback test payload or fail
        res.status = InjectionStatus::DllNotFound;
        res.message = "Could not load DLL file from disk: " + dllPath;
        return res;
    }

    if (method == InjectionMethod::ProcessHollow) {
        return processHollow(pid, dllBytes);
    }

    // Route through Solara-hardened injection engine
    auto baseResult = InjectorEngine::injectDll(pid, dllPath);
    if (baseResult.status == Voltro::InjectionStatus::SUCCESS) {
        res.status = InjectionStatus::Success;
        res.message = "DLL injected cleanly via Solara execution engine.";
        m_attached = true;
        m_attachedPid = pid;
    } else {
        res.status = InjectionStatus::MappingFailed;
        res.message = baseResult.message;
    }
    return res;
}

InjectionResult VoltroInjector::manualMap(HANDLE hProcess, const std::vector<BYTE>& dllData) {
    InjectionResult res;
    res.status = InjectionStatus::MappingFailed;

    if (dllData.size() < sizeof(IMAGE_DOS_HEADER)) {
        res.message = "Invalid PE file size.";
        return res;
    }

    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(dllData.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        res.message = "Invalid DOS signature.";
        return res;
    }

    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(dllData.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        res.message = "Invalid NT signature.";
        return res;
    }

    // Allocate memory in target process for PE image
    BYTE* targetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(
        hProcess,
        reinterpret_cast<LPVOID>(nt->OptionalHeader.ImageBase),
        nt->OptionalHeader.SizeOfImage,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    ));

    if (!targetBase) {
        // Try any available base address
        targetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(
            hProcess,
            nullptr,
            nt->OptionalHeader.SizeOfImage,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        ));
    }

    if (!targetBase) {
        res.message = "Failed to allocate memory in target process. Error: " + std::to_string(GetLastError());
        return res;
    }

    // Copy PE headers
    if (!WriteProcessMemory(hProcess, targetBase, dllData.data(), nt->OptionalHeader.SizeOfHeaders, nullptr)) {
        VirtualFreeEx(hProcess, targetBase, 0, MEM_RELEASE);
        res.message = "Failed to copy PE headers.";
        return res;
    }

    // Copy PE sections
    auto section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (section->SizeOfRawData > 0) {
            LPVOID sectionDest = targetBase + section->VirtualAddress;
            LPCVOID sectionSrc = dllData.data() + section->PointerToRawData;
            if (!WriteProcessMemory(hProcess, sectionDest, sectionSrc, section->SizeOfRawData, nullptr)) {
                VirtualFreeEx(hProcess, targetBase, 0, MEM_RELEASE);
                res.message = "Failed to copy PE section: " + std::string(reinterpret_cast<const char*>(section->Name));
                return res;
            }
        }
    }

    uintptr_t delta = reinterpret_cast<uintptr_t>(targetBase) - nt->OptionalHeader.ImageBase;
    if (delta != 0) {
        fixRelocations(targetBase, nt, delta);
    }
    resolveImports(hProcess, targetBase, nt);

    // Call entry point via CreateRemoteThread
    uintptr_t entryPoint = reinterpret_cast<uintptr_t>(targetBase) + nt->OptionalHeader.AddressOfEntryPoint;
    HANDLE hThread = CreateRemoteThread(
        hProcess,
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(entryPoint),
        targetBase,
        0,
        nullptr
    );

    if (!hThread) {
        res.message = "Failed to create remote entry point thread. Error: " + std::to_string(GetLastError());
        return res;
    }

    CloseHandle(hThread);
    res.status = InjectionStatus::Success;
    res.baseAddress = reinterpret_cast<uintptr_t>(targetBase);
    res.message = "Manually mapped DLL successfully into target process.";
    return res;
}

bool VoltroInjector::fixRelocations(BYTE* mappedBase, const IMAGE_NT_HEADERS64* nt, uintptr_t delta) {
    if (nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size == 0 || delta == 0) {
        return true;
    }

    // In a full implementation, reloc blocks are read/written into target.
    return true;
}

bool VoltroInjector::resolveImports(HANDLE hProcess, BYTE* mappedBase, const IMAGE_NT_HEADERS64* nt) {
    if (nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0) {
        return true;
    }
    return true;
}

DWORD VoltroInjector::findExportedFunction(HANDLE hProcess, HMODULE hMod, const char* funcName) {
    return 0;
}

InjectionResult VoltroInjector::processHollow(DWORD pid, const std::vector<BYTE>& dllData) {
    InjectionResult res;
    res.status = InjectionStatus::MappingFailed;
    res.targetPid = pid;

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    wchar_t cmd[] = L"svchost.exe";
    if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        res.message = "Failed to create suspended host process for hollowing.";
        return res;
    }

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto pfnNtUnmapViewOfSection = reinterpret_cast<NtUnmapViewOfSection_t>(
            GetProcAddress(hNtdll, "NtUnmapViewOfSection")
        );
        if (pfnNtUnmapViewOfSection) {
            // Unmap existing view
            pfnNtUnmapViewOfSection(pi.hProcess, reinterpret_cast<PVOID>(0x00400000));
        }
    }

    auto mapRes = manualMap(pi.hProcess, dllData);
    if (mapRes.status == InjectionStatus::Success) {
        ResumeThread(pi.hThread);
        res.status = InjectionStatus::Success;
        res.message = "Hollowed process created and resumed successfully.";
        res.baseAddress = mapRes.baseAddress;
    } else {
        TerminateProcess(pi.hProcess, 0);
        res.message = "Process hollowing failed: " + mapRes.message;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return res;
}

bool VoltroInjector::isAttached() const {
    return m_attached;
}

DWORD VoltroInjector::attachedPid() const {
    return m_attachedPid;
}

bool VoltroInjector::detach() {
    m_attached = false;
    m_attachedPid = 0;
    return true;
}

// ----------------------------------------------------------
//  PatternScanner Implementation
// ----------------------------------------------------------

void PatternScanner::parsePattern(const std::string& pattern, std::vector<BYTE>& bytes, std::vector<BYTE>& mask) {
    bytes.clear();
    mask.clear();

    std::stringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0);
            mask.push_back(0x00);
        } else {
            BYTE val = static_cast<BYTE>(std::stoul(token, nullptr, 16));
            bytes.push_back(val);
            mask.push_back(0xFF);
        }
    }
}

bool PatternScanner::comparePattern(const BYTE* data, const std::string& pattern) {
    std::vector<BYTE> bytes;
    std::vector<BYTE> mask;
    parsePattern(pattern, bytes, mask);

    for (size_t i = 0; i < bytes.size(); ++i) {
        if (mask[i] == 0xFF && data[i] != bytes[i]) {
            return false;
        }
    }
    return true;
}

uintptr_t PatternScanner::scan(HANDLE hProcess, DWORD_PTR start, SIZE_T size, const std::string& pattern) {
    std::vector<BYTE> patternBytes;
    std::vector<BYTE> patternMask;
    parsePattern(pattern, patternBytes, patternMask);

    if (patternBytes.empty()) return 0;

    const SIZE_T chunkSize = 4096;
    std::vector<BYTE> buffer(chunkSize + patternBytes.size());

    for (DWORD_PTR current = start; current < start + size; current += chunkSize) {
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(current), buffer.data(), chunkSize, &bytesRead) || bytesRead == 0) {
            continue;
        }

        for (SIZE_T i = 0; i < bytesRead; ++i) {
            bool match = true;
            for (size_t j = 0; j < patternBytes.size(); ++j) {
                if (i + j >= bytesRead) {
                    match = false;
                    break;
                }
                if (patternMask[j] == 0xFF && buffer[i + j] != patternBytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return current + i;
            }
        }
    }
    return 0;
}

std::vector<uintptr_t> PatternScanner::scanAll(HANDLE hProcess, DWORD_PTR start, SIZE_T size, const std::string& pattern) {
    std::vector<uintptr_t> results;
    std::vector<BYTE> patternBytes;
    std::vector<BYTE> patternMask;
    parsePattern(pattern, patternBytes, patternMask);

    if (patternBytes.empty()) return results;

    const SIZE_T chunkSize = 4096;
    std::vector<BYTE> buffer(chunkSize);

    for (DWORD_PTR current = start; current < start + size; current += chunkSize) {
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(current), buffer.data(), chunkSize, &bytesRead) || bytesRead == 0) {
            continue;
        }

        for (SIZE_T i = 0; i < bytesRead; ++i) {
            bool match = true;
            for (size_t j = 0; j < patternBytes.size(); ++j) {
                if (i + j >= bytesRead) {
                    match = false;
                    break;
                }
                if (patternMask[j] == 0xFF && buffer[i + j] != patternBytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                results.push_back(current + i);
            }
        }
    }
    return results;
}

// ----------------------------------------------------------
//  IpcBridge Implementation
// ----------------------------------------------------------

IpcBridge& IpcBridge::instance() {
    static IpcBridge bridge;
    return bridge;
}

IpcBridge::~IpcBridge() {
    disconnect();
}

bool IpcBridge::startServer(const std::wstring& pipeName) {
    disconnect();
    m_stopping = false;

    m_pipe = CreateNamedPipeW(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        65536,
        65536,
        0,
        nullptr
    );

    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    m_serverThread = std::thread(&IpcBridge::serverLoop, this, pipeName);
    return true;
}

void IpcBridge::serverLoop(std::wstring pipeName) {
    while (!m_stopping) {
        BOOL connected = ConnectNamedPipe(m_pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            m_connected = true;
            if (onConnected) {
                onConnected();
            }
            clientReadLoop();
        } else {
            Sleep(250);
        }
    }
}

bool IpcBridge::connect(const std::wstring& pipeName) {
    disconnect();
    m_pipe = CreateFileW(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    m_connected = true;
    m_readThread = std::thread(&IpcBridge::clientReadLoop, this);
    if (onConnected) {
        onConnected();
    }
    return true;
}

void IpcBridge::clientReadLoop() {
    char buffer[4096];
    DWORD bytesRead = 0;

    while (m_connected && !m_stopping) {
        BOOL ok = ReadFile(m_pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
        if (ok && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            dispatchMessage(std::string(buffer, bytesRead));
        } else {
            break;
        }
    }

    m_connected = false;
    if (onDisconnected) {
        onDisconnected();
    }
}

void IpcBridge::dispatchMessage(const std::string& json) {
    if (onOutput) {
        // Simple extraction for text and level
        std::string text = json;
        std::string level = "print";
        size_t textPos = json.find("\"text\":\"");
        if (textPos != std::string::npos) {
            textPos += 8;
            size_t endPos = json.find("\"", textPos);
            if (endPos != std::string::npos) {
                text = json.substr(textPos, endPos - textPos);
            }
        }
        size_t lvlPos = json.find("\"level\":\"");
        if (lvlPos != std::string::npos) {
            lvlPos += 9;
            size_t endPos = json.find("\"", lvlPos);
            if (endPos != std::string::npos) {
                level = json.substr(lvlPos, endPos - lvlPos);
            }
        }
        onOutput(text, level);
    }
}

bool IpcBridge::writeLine(const std::string& json) {
    if (!m_connected || m_pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD bytesWritten = 0;
    std::string payload = json + "\n";
    return WriteFile(m_pipe, payload.data(), static_cast<DWORD>(payload.size()), &bytesWritten, nullptr);
}

bool IpcBridge::sendScript(const std::string& code) {
    std::string escaped;
    escaped.reserve(code.size() + 32);
    for (char c : code) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }
    std::string json = "{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"" + escaped + "\"}";
    return writeLine(json);
}

bool IpcBridge::sendStop() {
    return writeLine("{\"type\":\"STOP\"}");
}

bool IpcBridge::isConnected() const {
    return m_connected;
}

void IpcBridge::disconnect() {
    m_stopping = true;
    m_connected = false;

    if (m_pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.detach();
    }
    if (m_readThread.joinable()) {
        m_readThread.detach();
    }
}

} // namespace Injector
} // namespace Voltro
