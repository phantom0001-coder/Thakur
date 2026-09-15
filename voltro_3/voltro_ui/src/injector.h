#pragma once

#include <windows.h>
// Undefine Windows 'get' macro to avoid conflicts with Qt's QNetworkAccessManager::get()
#undef get

#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace Voltro {

// Extended status codes for Manual Mapping
enum class InjectionStatus {
    SUCCESS,
    PROCESS_NOT_FOUND,
    OPEN_PROCESS_FAILED,
    MEMORY_ALLOCATION_FAILED,
    WRITE_MEMORY_FAILED,
    REMOTE_THREAD_FAILED,
    INTEGRITY_CHECK_FAILED,
    PE_PARSING_FAILED,
    SECTION_MAPPING_FAILED,
    RELOCATION_FAILED,
    IAT_RESOLUTION_FAILED,
    ENTRY_POINT_FAILED,
    SECTION_STOMPING_FAILED,
    NT_SET_IO_COMPLETION_FAILED
};

struct InjectionResult {
    InjectionStatus status;
    std::string message;
    DWORD targetPid = 0;
};

class InjectorEngine {
public:
    // Get absolute path for DLL
    static std::string getDllAbsolutePath(const std::string& dllName);

    // inj-1: Manual Mapping with Section Stomping + NtSetIoCompletion
    static InjectionResult injectDll(DWORD pid, const std::string& dllPath);

    // inj-1: Manual Mapping from memory (raw PE bytes)
    static InjectionResult manualMapInject(DWORD pid, const std::vector<BYTE>& peData);

    // inj-1: Section Stomping - Hide code in legitimate system DLL section
    static InjectionResult sectionStompingInject(DWORD pid, const std::string& dllPath);

private:
    // Helper: Read PE file into memory
    static bool readPeFile(const std::string& filePath, std::vector<BYTE>& outData);

    // Helper: Perform manual mapping
    static InjectionResult performManualMapping(HANDLE hProcess, const std::vector<BYTE>& peData);

    // Helper: Resolve imports for manually mapped module
    static bool resolveImports(HANDLE hProcess, LPVOID mappedBase, 
                               DWORD importRva, DWORD importSize,
                               const std::vector<BYTE>& peData);

    // Helper: Find section for RVA
    static const IMAGE_SECTION_HEADER* findSectionForRva(
        const IMAGE_SECTION_HEADER* sections, DWORD numSections, DWORD rva);

    // Helper: Apply relocations
    static bool applyRelocations(HANDLE hProcess, LPVOID mappedBase, 
                                 ULONGLONG delta, DWORD relocRva, DWORD relocSize,
                                 const std::vector<BYTE>& peData);

    // Helper: Get NtSetIoCompletion from ntdll
    static FARPROC getNtSetIoCompletion();

    // Helper: Create shellcode for DllMain/EntryPoint execution
    static std::vector<BYTE> createEntryPointShellcode(ULONGLONG entryPoint, ULONGLONG baseAddress);
};

} // namespace Voltro
