#pragma once

#include <windows.h>

#undef get

#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace Voltro {


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
    
    static std::string getDllAbsolutePath(const std::string& dllName);

    
    static InjectionResult injectDll(DWORD pid, const std::string& dllPath);

    
    static InjectionResult manualMapInject(DWORD pid, const std::vector<BYTE>& peData);

    
    static InjectionResult sectionStompingInject(DWORD pid, const std::string& dllPath);

private:
    
    static bool readPeFile(const std::string& filePath, std::vector<BYTE>& outData);

    
    static InjectionResult performManualMapping(HANDLE hProcess, const std::vector<BYTE>& peData);

    
    static bool resolveImports(HANDLE hProcess, LPVOID mappedBase, 
                               DWORD importRva, DWORD importSize,
                               const std::vector<BYTE>& peData);

    
    static const IMAGE_SECTION_HEADER* findSectionForRva(
        const IMAGE_SECTION_HEADER* sections, DWORD numSections, DWORD rva);

    
    static bool applyRelocations(HANDLE hProcess, LPVOID mappedBase, 
                                 ULONGLONG delta, DWORD relocRva, DWORD relocSize,
                                 const std::vector<BYTE>& peData);

    
    static FARPROC getNtSetIoCompletion();

    
    static std::vector<BYTE> createEntryPointShellcode(ULONGLONG entryPoint, ULONGLONG baseAddress);
};

} 
