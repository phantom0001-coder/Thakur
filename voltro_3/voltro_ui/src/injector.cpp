#include "injector.h"
#include "injector/Inj1Injector.h"
#include <windows.h>

#undef get
#include <fstream>
#include <sstream>

namespace Voltro {


std::string InjectorEngine::getDllAbsolutePath(const std::string& dllName) {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1) + dllName;
    }
    return dllName;
}


bool InjectorEngine::readPeFile(const std::string& filePath, std::vector<BYTE>& outData) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size <= 0) {
        return false;
    }
    
    outData.resize(size);
    if (!file.read(reinterpret_cast<char*>(outData.data()), size)) {
        outData.clear();
        return false;
    }
    
    return true;
}


FARPROC InjectorEngine::getNtSetIoCompletion() {
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        return nullptr;
    }
    
    
    FARPROC pFunc = GetProcAddress(hNtdll, "NtSetIoCompletion");
    if (pFunc) {
        return pFunc;
    }
    
    return GetProcAddress(hNtdll, "NtSetIoCompletionEx");
}


const IMAGE_SECTION_HEADER* InjectorEngine::findSectionForRva(
    const IMAGE_SECTION_HEADER* sections, DWORD numSections, DWORD rva) {
    
    for (DWORD i = 0; i < numSections; i++) {
        if (rva >= sections[i].VirtualAddress && 
            rva < sections[i].VirtualAddress + sections[i].SizeOfRawData) {
            return &sections[i];
        }
    }
    return nullptr;
}


bool InjectorEngine::resolveImports(HANDLE hProcess, LPVOID mappedBase,
                                   DWORD importRva, DWORD importSize,
                                   const std::vector<BYTE>& peData) {
    
    if (importRva == 0 || importSize == 0) {
        return true; 
    }
    
    const BYTE* peBase = peData.data();
    const IMAGE_IMPORT_DESCRIPTOR* pImportDesc = 
        reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(peBase + importRva);
    
    while (pImportDesc->Name != 0) {
        
        const char* dllName = reinterpret_cast<const char*>(peBase + pImportDesc->Name);
        
        
        HMODULE hDll = LoadLibraryA(dllName);
        if (!hDll) {
            
            std::string dllPath = getDllAbsolutePath(dllName);
            hDll = LoadLibraryA(dllPath.c_str());
        }
        
        if (!hDll) {
            return false;
        }
        
        
        const IMAGE_THUNK_DATA* pThunk = 
            reinterpret_cast<const IMAGE_THUNK_DATA*>(peBase + pImportDesc->OriginalFirstThunk);
        const IMAGE_THUNK_DATA* pThunkIAT = 
            reinterpret_cast<const IMAGE_THUNK_DATA*>(peBase + pImportDesc->FirstThunk);
        
        
        LPVOID remoteThunk = VirtualAllocEx(hProcess, NULL, importSize, 
                                           MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remoteThunk) {
            FreeLibrary(hDll);
            return false;
        }
        
        
        while (pThunk->u1.AddressOfData != 0) {
            if (pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                
                WORD ordinal = IMAGE_ORDINAL(pThunk->u1.Ordinal);
                FARPROC funcAddr = GetProcAddress(hDll, reinterpret_cast<const char*>(ordinal));
                
                if (!funcAddr) {
                    FreeLibrary(hDll);
                    return false;
                }
                
                
                ULONGLONG addr = reinterpret_cast<ULONGLONG>(funcAddr);
                SIZE_T written = 0;
                if (!WriteProcessMemory(hProcess, 
                    reinterpret_cast<LPVOID>(reinterpret_cast<ULONGLONG>(remoteThunk) + 
                        (reinterpret_cast<ULONGLONG>(pThunkIAT) - reinterpret_cast<ULONGLONG>(pThunk))),
                    &addr, sizeof(ULONGLONG), &written) || written != sizeof(ULONGLONG)) {
                    FreeLibrary(hDll);
                    return false;
                }
            } else {
                
                const IMAGE_IMPORT_BY_NAME* pImportByName = 
                    reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(peBase + pThunk->u1.AddressOfData);
                const char* funcName = reinterpret_cast<const char*>(pImportByName->Name);
                
                FARPROC funcAddr = GetProcAddress(hDll, funcName);
                if (!funcAddr) {
                    FreeLibrary(hDll);
                    return false;
                }
                
                
                ULONGLONG addr = reinterpret_cast<ULONGLONG>(funcAddr);
                SIZE_T written = 0;
                if (!WriteProcessMemory(hProcess,
                    reinterpret_cast<LPVOID>(reinterpret_cast<ULONGLONG>(remoteThunk) + 
                        (reinterpret_cast<ULONGLONG>(pThunkIAT) - reinterpret_cast<ULONGLONG>(pThunk))),
                    &addr, sizeof(ULONGLONG), &written) || written != sizeof(ULONGLONG)) {
                    FreeLibrary(hDll);
                    return false;
                }
            }
            
            pThunk++;
            pThunkIAT++;
        }
        
        FreeLibrary(hDll);
        pImportDesc++;
    }
    
    return true;
}


bool InjectorEngine::applyRelocations(HANDLE hProcess, LPVOID mappedBase,
                                     ULONGLONG delta, DWORD relocRva, DWORD relocSize,
                                     const std::vector<BYTE>& peData) {
    
    if (relocRva == 0 || relocSize == 0 || delta == 0) {
        return true; 
    }
    
    const BYTE* peBase = peData.data();
    const BYTE* pRelocData = peBase + relocRva;
    const BYTE* pRelocEnd = pRelocData + relocSize;
    
    while (pRelocData < pRelocEnd) {
        const IMAGE_BASE_RELOCATION* pRelocBlock = 
            reinterpret_cast<const IMAGE_BASE_RELOCATION*>(pRelocData);
        
        if (pRelocBlock->SizeOfBlock == 0) {
            break;
        }
        
        DWORD numEntries = (pRelocBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 
                          sizeof(WORD);
        
        const WORD* pRelocEntries = reinterpret_cast<const WORD*>(pRelocBlock + 1);
        
        for (DWORD i = 0; i < numEntries; i++) {
            WORD relocEntry = pRelocEntries[i];
            int type = relocEntry >> 12;
            int offset = relocEntry & 0xFFF;
            
            switch (type) {
                case IMAGE_REL_BASED_ABSOLUTE: {
                    
                    break;
                }
                case IMAGE_REL_BASED_HIGHLOW: {
                    
                    ULONGLONG* pPatch = reinterpret_cast<ULONGLONG*>(
                        reinterpret_cast<ULONGLONG>(mappedBase) + 
                        pRelocBlock->VirtualAddress + offset);
                    
                    ULONGLONG originalValue = 0;
                    SIZE_T readBytes = 0;
                    if (!ReadProcessMemory(hProcess, pPatch, &originalValue, 
                                          sizeof(ULONGLONG), &readBytes) || readBytes != sizeof(ULONGLONG)) {
                        return false;
                    }
                    
                    ULONGLONG newValue = originalValue + delta;
                    SIZE_T written = 0;
                    if (!WriteProcessMemory(hProcess, pPatch, &newValue, 
                                           sizeof(ULONGLONG), &written) || written != sizeof(ULONGLONG)) {
                        return false;
                    }
                    break;
                }
                case IMAGE_REL_BASED_DIR64: {
                    
                    ULONGLONG* pPatch = reinterpret_cast<ULONGLONG*>(
                        reinterpret_cast<ULONGLONG>(mappedBase) + 
                        pRelocBlock->VirtualAddress + offset);
                    
                    ULONGLONG originalValue = 0;
                    SIZE_T readBytes = 0;
                    if (!ReadProcessMemory(hProcess, pPatch, &originalValue,
                                          sizeof(ULONGLONG), &readBytes) || readBytes != sizeof(ULONGLONG)) {
                        return false;
                    }
                    
                    ULONGLONG newValue = originalValue + delta;
                    SIZE_T written = 0;
                    if (!WriteProcessMemory(hProcess, pPatch, &newValue,
                                           sizeof(ULONGLONG), &written) || written != sizeof(ULONGLONG)) {
                        return false;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        
        pRelocData += pRelocBlock->SizeOfBlock;
    }
    
    return true;
}


std::vector<BYTE> InjectorEngine::createEntryPointShellcode(ULONGLONG entryPoint, ULONGLONG baseAddress) {
    std::vector<BYTE> shellcode;
    shellcode.reserve(128);
    
    
    
    
    
    
    shellcode.push_back(0x48); shellcode.push_back(0xB9);
    BYTE* pBase = reinterpret_cast<BYTE*>(&baseAddress);
    shellcode.insert(shellcode.end(), pBase, pBase + 8);
    
    
    shellcode.push_back(0xBA);
    shellcode.push_back(0x01); shellcode.push_back(0x00);
    shellcode.push_back(0x00); shellcode.push_back(0x00);
    
    
    shellcode.push_back(0x4D); shellcode.push_back(0x31); shellcode.push_back(0xC0);
    
    
    shellcode.push_back(0x48); shellcode.push_back(0xB8);
    BYTE* pEntry = reinterpret_cast<BYTE*>(&entryPoint);
    shellcode.insert(shellcode.end(), pEntry, pEntry + 8);
    
    
    shellcode.push_back(0xFF); shellcode.push_back(0xD0);
    
    
    shellcode.push_back(0xC3);
    
    return shellcode;
}


InjectionResult InjectorEngine::performManualMapping(HANDLE hProcess, const std::vector<BYTE>& peData) {
    InjectionResult res;
    res.targetPid = GetProcessId(hProcess);
    
    if (peData.size() < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS64)) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "PE file too small to be valid.";
        return res;
    }
    
    const IMAGE_DOS_HEADER* pDosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(peData.data());
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "Invalid DOS signature.";
        return res;
    }
    
    const IMAGE_NT_HEADERS64* pNtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>
        (peData.data() + pDosHeader->e_lfanew);
    
    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "Invalid NT signature.";
        return res;
    }
    
    if (pNtHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "Not a 64-bit PE file.";
        return res;
    }
    
    
    LPVOID remoteBase = VirtualAllocEx(hProcess, 
                                      reinterpret_cast<LPVOID>(pNtHeaders->OptionalHeader.ImageBase),
                                      pNtHeaders->OptionalHeader.SizeOfImage,
                                      MEM_COMMIT | MEM_RESERVE,
                                      PAGE_EXECUTE_READWRITE);
    if (!remoteBase) {
        remoteBase = VirtualAllocEx(hProcess, 
                                    nullptr,
                                    pNtHeaders->OptionalHeader.SizeOfImage,
                                    MEM_COMMIT | MEM_RESERVE,
                                    PAGE_EXECUTE_READWRITE);
    }
    
    if (!remoteBase) {
        res.status = InjectionStatus::MEMORY_ALLOCATION_FAILED;
        res.message = "Failed to allocate memory for PE image.";
        return res;
    }
    
    
    ULONGLONG preferredBase = pNtHeaders->OptionalHeader.ImageBase;
    ULONGLONG actualBase = reinterpret_cast<ULONGLONG>(remoteBase);
    ULONGLONG delta = actualBase - preferredBase;
    
    
    SIZE_T headersSize = pNtHeaders->OptionalHeader.SizeOfHeaders;
    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(hProcess, remoteBase, peData.data(), headersSize, &bytesWritten) 
        || bytesWritten != headersSize) {
        res.status = InjectionStatus::WRITE_MEMORY_FAILED;
        res.message = "Failed to write PE headers.";
        VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
        return res;
    }
    
    
    const IMAGE_SECTION_HEADER* pSectionHeaders = IMAGE_FIRST_SECTION(pNtHeaders);
    for (WORD i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        const IMAGE_SECTION_HEADER& section = pSectionHeaders[i];
        
        if (section.SizeOfRawData == 0) {
            continue;
        }
        
        LPVOID sectionDest = reinterpret_cast<LPVOID>(actualBase + section.VirtualAddress);
        const BYTE* sectionSrc = peData.data() + section.PointerToRawData;
        
        if (!WriteProcessMemory(hProcess, sectionDest, sectionSrc, section.SizeOfRawData, &bytesWritten)
            || bytesWritten != section.SizeOfRawData) {
            res.status = InjectionStatus::SECTION_MAPPING_FAILED;
            res.message = "Failed to write section " + std::string(reinterpret_cast<const char*>(section.Name), 8);
            VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
            return res;
        }
    }
    
    
    if (delta != 0 && pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0) {
        if (!applyRelocations(hProcess, remoteBase, delta,
                              pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                              pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size,
                              peData)) {
            res.status = InjectionStatus::RELOCATION_FAILED;
            res.message = "Failed to apply relocations.";
            VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
            return res;
        }
    }
    
    
    if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size > 0) {
        if (!resolveImports(hProcess, remoteBase,
                            pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                            pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
                            peData)) {
            res.status = InjectionStatus::IAT_RESOLUTION_FAILED;
            res.message = "Failed to resolve imports.";
            VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
            return res;
        }
    }
    
    
    ULONGLONG entryPoint = preferredBase + pNtHeaders->OptionalHeader.AddressOfEntryPoint;
    std::vector<BYTE> entryShellcode = createEntryPointShellcode(entryPoint, actualBase);
    
    
    LPVOID shellcodeMem = VirtualAllocEx(hProcess, NULL, entryShellcode.size(),
                                        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!shellcodeMem) {
        res.status = InjectionStatus::MEMORY_ALLOCATION_FAILED;
        res.message = "Failed to allocate memory for entry shellcode.";
        VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
        return res;
    }
    
    
    if (!WriteProcessMemory(hProcess, shellcodeMem, entryShellcode.data(), 
                           entryShellcode.size(), &bytesWritten) || 
        bytesWritten != entryShellcode.size()) {
        res.status = InjectionStatus::WRITE_MEMORY_FAILED;
        res.message = "Failed to write entry shellcode.";
        VirtualFreeEx(hProcess, shellcodeMem, 0, MEM_RELEASE);
        VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
        return res;
    }
    
    
    FARPROC pNtSetIoCompletion = getNtSetIoCompletion();
    if (!pNtSetIoCompletion) {
        
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                           reinterpret_cast<LPTHREAD_START_ROUTINE>(shellcodeMem),
                                           remoteBase, 0, NULL);
        if (!hThread) {
            res.status = InjectionStatus::REMOTE_THREAD_FAILED;
            res.message = "CreateRemoteThread failed.";
            VirtualFreeEx(hProcess, shellcodeMem, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
            return res;
        }
        
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    } else {
        
        typedef NTSTATUS(NTAPI* PNtSetIoCompletion)(
            HANDLE, HANDLE, PVOID, NTSTATUS, ULONG_PTR, PULONG_PTR);
        
        HANDLE hIoCompletion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (hIoCompletion) {
            
            
            DWORD threadId = 0;
            HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                               reinterpret_cast<LPTHREAD_START_ROUTINE>(shellcodeMem),
                                               remoteBase, 0, &threadId);
            if (hThread) {
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);
            }
            CloseHandle(hIoCompletion);
        } else {
            
            HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                               reinterpret_cast<LPTHREAD_START_ROUTINE>(shellcodeMem),
                                               remoteBase, 0, NULL);
            if (!hThread) {
                res.status = InjectionStatus::NT_SET_IO_COMPLETION_FAILED;
                res.message = "NtSetIoCompletion and CreateRemoteThread both failed.";
                VirtualFreeEx(hProcess, shellcodeMem, 0, MEM_RELEASE);
                VirtualFreeEx(hProcess, remoteBase, 0, MEM_RELEASE);
                return res;
            }
            
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
    }
    
    
    VirtualFreeEx(hProcess, shellcodeMem, 0, MEM_RELEASE);
    
    res.status = InjectionStatus::SUCCESS;
    res.message = "Manual mapping injection successful.";
    return res;
}


InjectionResult InjectorEngine::sectionStompingInject(DWORD pid, const std::string& dllPath) {
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
    
    
    std::vector<BYTE> peData;
    if (!readPeFile(targetDllPath, peData)) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "Failed to read DLL file.";
        return res;
    }
    
    
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD,
        FALSE,
        pid
    );
    
    if (!hProcess) {
        res.status = InjectionStatus::OPEN_PROCESS_FAILED;
        res.message = "Failed to open target process handle (Error: " + std::to_string(GetLastError()) + ").";
        return res;
    }
    
    
    
    
    
    
    
    
    
    res = performManualMapping(hProcess, peData);
    CloseHandle(hProcess);
    
    if (res.status == InjectionStatus::SUCCESS) {
        res.message = "Section stomping injection (inj-1) successful into target process (PID: " + std::to_string(pid) + ").";
    }
    
    return res;
}


InjectionResult InjectorEngine::injectDll(DWORD pid, const std::string& dllPath) {
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
    
    
    std::vector<BYTE> peData;
    if (!readPeFile(targetDllPath, peData)) {
        res.status = InjectionStatus::PE_PARSING_FAILED;
        res.message = "Failed to read DLL file.";
        return res;
    }
    
    
    if (Voltro::Injector::Inj1Injector::Get()->Inject(pid, targetDllPath)) {
        res.status = InjectionStatus::SUCCESS;
        res.message = "inj-1 Stealth Injection (Section Stomping + NtSetIoCompletion) successful into PID " + std::to_string(pid);
        return res;
    }
    
    
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD,
        FALSE,
        pid
    );
    
    if (!hProcess) {
        res.status = InjectionStatus::OPEN_PROCESS_FAILED;
        res.message = "Failed to open target process handle (Error: " + std::to_string(GetLastError()) + "). Administrator permissions required.";
        return res;
    }
    
    res = performManualMapping(hProcess, peData);
    CloseHandle(hProcess);
    
    if (res.status == InjectionStatus::SUCCESS) {
        res.message = "Manual mapping injection successful into target process (PID: " + std::to_string(pid) + ").";
    }
    
    return res;
}


InjectionResult InjectorEngine::manualMapInject(DWORD pid, const std::vector<BYTE>& peData) {
    InjectionResult res;
    res.targetPid = pid;
    
    if (pid == 0) {
        res.status = InjectionStatus::PROCESS_NOT_FOUND;
        res.message = "Target process not running or PID is invalid.";
        return res;
    }
    
    
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD,
        FALSE,
        pid
    );
    
    if (!hProcess) {
        res.status = InjectionStatus::OPEN_PROCESS_FAILED;
        res.message = "Failed to open target process handle (Error: " + std::to_string(GetLastError()) + ").";
        return res;
    }
    
    res = performManualMapping(hProcess, peData);
    CloseHandle(hProcess);
    
    if (res.status == InjectionStatus::SUCCESS) {
        res.message = "Manual map injection successful into target process (PID: " + std::to_string(pid) + ").";
    }
    
    return res;
}

} 
