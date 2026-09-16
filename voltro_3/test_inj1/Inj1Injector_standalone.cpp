




#include "Inj1Injector_standalone.h"
#include <iostream>
#include <fstream>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")


bool IsModuleLoaded(DWORD Pid, const char* ModuleName)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Pid);
    if (!hProcess) return false;

    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            char szModName[MAX_PATH];
            if (GetModuleBaseNameA(hProcess, hMods[i], szModName, sizeof(szModName)))
            {
                if (_stricmp(szModName, ModuleName) == 0)
                {
                    CloseHandle(hProcess);
                    return true;
                }
            }
        }
    }

    CloseHandle(hProcess);
    return false;
}





bool Inj1Injector::Inject(DWORD Pid, const std::string& dllPath)
{
    this->Pid = Pid;
    this->m_dllPath = dllPath;
    
    SuspendProcess(Pid);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Pid);
    if (!hProcess)
    {
        ResumeProcess(Pid);
        return false;
    }

    auto Nt = GetNtFunctions();
    if (!Nt.CreateSection || !Nt.MapViewOfSection || !Nt.WriteVirtualMemory)
    {
        CloseHandle(hProcess);
        ResumeProcess(Pid);
        return false;
    }

    
    auto Raw = LoadTarget(dllPath);
    if (!Raw)
    {
        CloseHandle(hProcess);
        ResumeProcess(Pid);
        return false;
    }

    auto* pNt = (PIMAGE_NT_HEADERS64)(Raw + ((PIMAGE_DOS_HEADER)Raw)->e_lfanew);

    DWORD EntryPointRVA = pNt->OptionalHeader.AddressOfEntryPoint;
    auto& Imp = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto& Exc = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    auto& Tls = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];

    ULONG64 ImportRVA = Imp.VirtualAddress;
    ULONG64 ImportSize = Imp.Size;
    ULONG64 ExceptionRVA = Exc.VirtualAddress;
    ULONG64 ExceptionSize = Exc.Size;
    ULONG64 TlsRVA = Tls.VirtualAddress;
    ULONG64 TlsSize = Tls.Size;

    
    auto DllPath = FindTargetDll(Nt);
    if (DllPath.empty())
    {
        free(Raw);
        CloseHandle(hProcess);
        ResumeProcess(Pid);
        return false;
    }

    
    uintptr_t Base = MapTarget(hProcess, Nt, pNt, Raw, DllPath);
    free(Raw);

    if (!Base)
    {
        CloseHandle(hProcess);
        ResumeProcess(Pid);
        return false;
    }

    
    bool Result = ExecuteLoader(hProcess, Nt, Base, EntryPointRVA,
        ImportRVA, ImportSize, ExceptionRVA, ExceptionSize, TlsRVA, TlsSize);

    CloseHandle(hProcess);
    ResumeProcess(Pid);

    return Result;
}

DWORD Inj1Injector::GetProcessPidByName(const char* Name)
{
    DWORD Pid = 0;
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 Entry = {};
    Entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(Snapshot, &Entry))
    {
        do
        {
            if (_stricmp(Entry.szExeFile, Name) == 0)
            {
                Pid = Entry.th32ProcessID;
                break;
            }
        } while (Process32Next(Snapshot, &Entry));
    }

    CloseHandle(Snapshot);
    return Pid;
}

void Inj1Injector::SuspendProcess(DWORD Pid)
{
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Snapshot == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 Entry;
    Entry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(Snapshot, &Entry))
    {
        do
        {
            if (Entry.th32OwnerProcessID == Pid)
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, Entry.th32ThreadID);
                if (hThread) {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(Snapshot, &Entry));
    }
    CloseHandle(Snapshot);
}

void Inj1Injector::ResumeProcess(DWORD Pid)
{
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Snapshot == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 Entry;
    Entry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(Snapshot, &Entry))
    {
        do
        {
            if (Entry.th32OwnerProcessID == Pid)
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, Entry.th32ThreadID);
                if (hThread)
                {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(Snapshot, &Entry));
    }
    CloseHandle(Snapshot);
}

Inj1Injector::NtFunctions Inj1Injector::GetNtFunctions()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

    return {
        (NtCreateSection_t)GetProcAddress(ntdll, "NtCreateSection"),
        (NtMapViewOfSection_t)GetProcAddress(ntdll, "NtMapViewOfSection"),
        (NtWriteVirtualMemory_t)GetProcAddress(ntdll, "NtWriteVirtualMemory"),
        (NtUnmapViewOfSection_t)GetProcAddress(ntdll, "NtUnmapViewOfSection"),
        (NtQuerySystemInformation_t)GetProcAddress(ntdll, "NtQuerySystemInformation"),
        (NtDuplicateObject_t)GetProcAddress(ntdll, "NtDuplicateObject"),
        (NtQueryObject_t)GetProcAddress(ntdll, "NtQueryObject"),
        (NtSetIoCompletion_t)GetProcAddress(ntdll, "NtSetIoCompletion"),
    };
}

uint8_t* Inj1Injector::LoadTarget(const std::string& dllPath)
{
    
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return nullptr;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
        return nullptr;

    uint8_t* Data = (uint8_t*)malloc(size);
    if (!Data)
        return nullptr;

    if (!file.read(reinterpret_cast<char*>(Data), size))
    {
        free(Data);
        return nullptr;
    }

    PIMAGE_DOS_HEADER Dos = (PIMAGE_DOS_HEADER)Data;
    if (Dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        free(Data);
        return nullptr;
    }

    PIMAGE_NT_HEADERS64 Nt = (PIMAGE_NT_HEADERS64)(Data + Dos->e_lfanew);
    if (Nt->Signature != IMAGE_NT_SIGNATURE)
    {
        free(Data);
        return nullptr;
    }

    return Data;
}

std::string Inj1Injector::FindTargetDll(const NtFunctions& Nt)
{
    std::vector<std::string> Dlls;
    WIN32_FIND_DATAA Ffd;
    HANDLE hFind = FindFirstFileA("C:\\Windows\\System32\\*.dll", &Ffd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (Ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (Ffd.nFileSizeHigh || Ffd.nFileSizeLow > 0xFFFF) continue;

            char Path[MAX_PATH];
            sprintf_s(Path, "C:\\Windows\\System32\\%s", Ffd.cFileName);

            std::string Name = Ffd.cFileName;
            
            if (Name.find("msvcp") == 0 ||
                Name.find("vcruntime") == 0 ||
                Name.find("msvcr") == 0 ||
                Name.find("concrt") == 0)
            {
                continue;
            }

            HANDLE hFile = CreateFileA(Path, GENERIC_READ | GENERIC_EXECUTE,
                FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, nullptr);

            if (hFile == INVALID_HANDLE_VALUE) continue;

            HANDLE hSec = nullptr;
            NTSTATUS st = Nt.CreateSection(&hSec, SECTION_ALL_ACCESS, nullptr, nullptr,
                PAGE_READONLY, SEC_IMAGE, hFile);
            CloseHandle(hFile);

            if (!NT_SUCCESS(st)) continue;

            PVOID Base = nullptr;
            SIZE_T Vsz = 0;
            st = Nt.MapViewOfSection(hSec, GetCurrentProcess(), &Base,
                0, 0, nullptr, &Vsz, 2, 0, PAGE_READONLY);

            if (NT_SUCCESS(st) && Vsz == 0x10000)
            {
                Dlls.push_back(Path);
                Nt.UnmapViewOfSection(GetCurrentProcess(), Base);
            }

            CloseHandle(hSec);
        } while (FindNextFileA(hFind, &Ffd));
        FindClose(hFind);
    }

    if (Dlls.empty()) return "";

    std::random_device Rd;
    std::mt19937 Gen(Rd());
    return Dlls[Gen() % Dlls.size()];
}

uintptr_t Inj1Injector::MapTarget(HANDLE hProcess, const NtFunctions& Nt,
                                  PIMAGE_NT_HEADERS64 pNt, uint8_t* Raw, const std::string& DllPath)
{
    const SIZE_T CHUNK = 0x10000;
    SIZE_T ImgSz = pNt->OptionalHeader.SizeOfImage;
    SIZE_T Aligned = (ImgSz + CHUNK - 1) & ~(CHUNK - 1);
    SIZE_T nChunks = Aligned / CHUNK;

    MEMORY_BASIC_INFORMATION Mbi = {};
    ULONG64 Base = 0;

    for (PBYTE p = (PBYTE)0x10000; VirtualQueryEx(hProcess, p, &Mbi, sizeof(Mbi));
        p = (PBYTE)Mbi.BaseAddress + Mbi.RegionSize)
    {
        if (Mbi.State != MEM_FREE || Mbi.RegionSize < Aligned) continue;
        ULONG64 a = ((ULONG64)Mbi.BaseAddress + 0xFFFF) & ~(ULONG64)0xFFFF;
        if (a + Aligned <= (ULONG64)Mbi.BaseAddress + Mbi.RegionSize)
        {
            Base = a;
            break;
        }
    }

    if (!Base) return 0;

    HANDLE hFile = CreateFileA(DllPath.c_str(), GENERIC_READ | GENERIC_EXECUTE,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) return 0;

    for (SIZE_T i = 0; i < nChunks; i++)
    {
        HANDLE hSec = nullptr;
        if (NT_SUCCESS(Nt.CreateSection(&hSec, SECTION_ALL_ACCESS, nullptr, nullptr,
            PAGE_READONLY, SEC_IMAGE, hFile)))
        {
            PVOID Addr = (PVOID)(Base + i * CHUNK);
            SIZE_T Vsz = 0;
            if (NT_SUCCESS(Nt.MapViewOfSection(hSec, hProcess, &Addr,
                0, 0, nullptr, &Vsz, 2, 0, PAGE_READONLY)))
            {
                DWORD Old = 0;
                VirtualProtectEx(hProcess, Addr, CHUNK, PAGE_EXECUTE_READWRITE, &Old);
            }
            CloseHandle(hSec);
        }
    }
    CloseHandle(hFile);

    uint8_t* Zeros = (uint8_t*)calloc(Aligned, 1);
    Nt.WriteVirtualMemory(hProcess, (PVOID)Base, Zeros, Aligned, nullptr);
    free(Zeros);

    uint8_t* Img = (uint8_t*)calloc(Aligned, 1);
    memcpy(Img, Raw, pNt->OptionalHeader.SizeOfHeaders);

    PIMAGE_SECTION_HEADER Sec = IMAGE_FIRST_SECTION(pNt);
    for (WORD s = 0; s < pNt->FileHeader.NumberOfSections; s++)
    {
        if (!Sec[s].SizeOfRawData) continue;
        memcpy(Img + Sec[s].VirtualAddress,
            Raw + Sec[s].PointerToRawData,
            Sec[s].SizeOfRawData);
    }

    INT64 Delta = (INT64)(Base - pNt->OptionalHeader.ImageBase);
    if (Delta)
    {
        auto& Dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (Dir.VirtualAddress && Dir.Size)
        {
            auto* Blk = (PIMAGE_BASE_RELOCATION)(Img + Dir.VirtualAddress);
            PBYTE End = Img + Dir.VirtualAddress + Dir.Size;

            while ((PBYTE)Blk < End && Blk->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
            {
                DWORD Cnt = (Blk->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                PWORD Ent = (PWORD)((PBYTE)Blk + sizeof(IMAGE_BASE_RELOCATION));

                for (DWORD i = 0; i < Cnt; i++)
                {
                    if ((Ent[i] & 0xF000) == (IMAGE_REL_BASED_DIR64 << 12))
                        *(ULONG64*)(Img + Blk->VirtualAddress + (Ent[i] & 0xFFF)) += Delta;
                }
                Blk = (PIMAGE_BASE_RELOCATION)((PBYTE)Blk + Blk->SizeOfBlock);
            }
        }
    }

    Nt.WriteVirtualMemory(hProcess, (PVOID)Base, Img, ImgSz, nullptr);
    free(Img);

    for (WORD s = 0; s < pNt->FileHeader.NumberOfSections; s++)
    {
        DWORD f = Sec[s].Characteristics;
        DWORD Prot = (f & IMAGE_SCN_MEM_EXECUTE) ? PAGE_EXECUTE_READ
            : (f & IMAGE_SCN_MEM_WRITE) ? PAGE_READWRITE : PAGE_READONLY;
        DWORD Old = 0;
        VirtualProtectEx(hProcess, (PVOID)(Base + Sec[s].VirtualAddress),
            Sec[s].Misc.VirtualSize, Prot, &Old);
    }

    return Base;
}

bool Inj1Injector::ExecuteLoader(HANDLE hProcess, const NtFunctions& Nt, uintptr_t Base,
                                 DWORD EntryPointRVA, ULONG64 ImportRVA, ULONG64 ImportSize,
                                 ULONG64 ExceptionRVA, ULONG64 ExceptionSize,
                                 ULONG64 TlsRVA, ULONG64 TlsSize)
{
    LoaderContext Ctx = {};
    Ctx.ImageBase = (PVOID)Base;
    Ctx.LoadLibraryA = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    Ctx.GetProcAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
    Ctx.RtlAddFunctionTable = GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlAddFunctionTable");

    Ctx.EntryPoint = EntryPointRVA ? (PVOID)(Base + EntryPointRVA) : nullptr;
    Ctx.ImportDirRVA = ImportRVA;
    Ctx.ImportDirSize = ImportSize;
    Ctx.ExceptionDirRVA = ExceptionRVA;
    Ctx.ExceptionDirSize = ExceptionSize;
    Ctx.TLSDirRVA = TlsRVA;
    Ctx.TLSDirSize = TlsSize;

    const SIZE_T LSIZ = sizeof(LoaderShellcode);
    const SIZE_T LALIGN = (LSIZ + 15) & ~(SIZE_T)15;
    const SIZE_T SECSZ = LALIGN + sizeof(LoaderContext) + 22 + 352;

    LARGE_INTEGER Sz = { (LONGLONG)SECSZ };
    HANDLE hSec = nullptr;
    NTSTATUS st = Nt.CreateSection(&hSec, SECTION_ALL_ACCESS, nullptr, &Sz,
        PAGE_EXECUTE_READWRITE, SEC_COMMIT, nullptr);

    if (!NT_SUCCESS(st)) return false;

    PVOID SecBase = nullptr;
    SIZE_T Vsz = SECSZ;
    st = Nt.MapViewOfSection(hSec, hProcess, &SecBase, 0, 0, nullptr,
        &Vsz, 2, 0, PAGE_EXECUTE_READWRITE);
    CloseHandle(hSec);

    if (!NT_SUCCESS(st)) return false;

    PBYTE LoaderAddr = (PBYTE)SecBase;
    PBYTE CtxAddr = LoaderAddr + LALIGN;
    PBYTE TramAddr = CtxAddr + sizeof(LoaderContext);

    BYTE Tramp[22];
    Tramp[0] = 0x48;
    Tramp[1] = 0xB8;
    memcpy(Tramp + 2, &LoaderAddr, 8);
    Tramp[10] = 0x48;
    Tramp[11] = 0xB9;
    memcpy(Tramp + 12, &CtxAddr, 8);
    Tramp[20] = 0xFF;
    Tramp[21] = 0xE0;

    SIZE_T BytesWritten = 0;

    st = Nt.WriteVirtualMemory(hProcess, LoaderAddr, (PVOID)LoaderShellcode, LSIZ, &BytesWritten);
    if (!NT_SUCCESS(st) || BytesWritten != LSIZ) return false;

    st = Nt.WriteVirtualMemory(hProcess, CtxAddr, &Ctx, sizeof(Ctx), &BytesWritten);
    if (!NT_SUCCESS(st) || BytesWritten != sizeof(Ctx)) return false;

    st = Nt.WriteVirtualMemory(hProcess, TramAddr, Tramp, sizeof(Tramp), &BytesWritten);
    if (!NT_SUCCESS(st) || BytesWritten != sizeof(Tramp)) return false;

    bool Result = ExecuteShellcode(hProcess, Nt, Pid, TramAddr);
    if (Result)
    {
        std::thread([hProcess, SecBase, SECSZ, TramAddr]() {
            Sleep(500);

            DWORD OldProtect = 0;
            VirtualProtectEx(hProcess, SecBase, SECSZ, PAGE_NOACCESS, &OldProtect);
            VirtualProtectEx(hProcess, TramAddr, 22, PAGE_NOACCESS, &OldProtect);
        }).detach();
    }

    return Result;
}

bool Inj1Injector::ExecuteShellcode(HANDLE hProcess, const NtFunctions& Nt, DWORD Pid, PVOID TramAddr)
{
    ULONG BufSz = 0x400000, Needed = 0;
    PVOID pBuf = malloc(BufSz);

    while (Nt.QuerySystemInformation(16, pBuf, BufSz, &Needed) == STATUS_INFO_LENGTH_MISMATCH)
    {
        free(pBuf);
        BufSz = Needed + 0x100000;
        pBuf = malloc(BufSz);
    }

    if (!pBuf) return false;

    auto* Hi = (SystemHandleInfo*)pBuf;
    HANDLE hPort = nullptr;
    ULONG64 TypeBuf[64] = {};

    for (ULONG i = 0; i < Hi->NumberOfHandles && !hPort; i++)
    {
        if (Hi->Handles[i].UniqueProcessId != Pid) continue;

        HANDLE hLocal = nullptr;
        if (!NT_SUCCESS(Nt.DuplicateObject(hProcess,
            (HANDLE)(ULONG_PTR)Hi->Handles[i].HandleValue,
            GetCurrentProcess(), &hLocal, 0, 0, 2))) continue;

        memset(TypeBuf, 0, sizeof(TypeBuf));
        if (NT_SUCCESS(Nt.QueryObject(hLocal, 2, TypeBuf, sizeof(TypeBuf), nullptr)))
        {
            USHORT Len = (USHORT)(TypeBuf[0] & 0xFFFF);
            LPCWSTR Buf = (LPCWSTR)TypeBuf[1];

            if (Len >= 0x18 && Buf && wcsncmp(Buf, L"IoCompletion", 12) == 0)
                hPort = hLocal;
            else
                CloseHandle(hLocal);
        }
    }
    free(pBuf);

    if (!hPort) return false;

    MEMORY_BASIC_INFORMATION Mbi = {};
    PVOID Cave = nullptr;

    for (PBYTE p = nullptr; VirtualQueryEx(hProcess, p, &Mbi, sizeof(Mbi));
        p = (PBYTE)Mbi.BaseAddress + Mbi.RegionSize)
    {
        if (Mbi.State != MEM_COMMIT || Mbi.Protect != PAGE_READWRITE || Mbi.RegionSize < 0x48)
            continue;

        PBYTE Buf = (PBYTE)malloc(Mbi.RegionSize);
        SIZE_T Rd = 0;

        if (ReadProcessMemory(hProcess, Mbi.BaseAddress, Buf, Mbi.RegionSize, &Rd))
        {
            for (SIZE_T Off = 0; Off + 0x48 <= Rd; Off++)
            {
                SIZE_T z = 0;
                while (z < 0x48 && !Buf[Off + z]) z++;
                if (z >= 0x48)
                {
                    Cave = (PBYTE)Mbi.BaseAddress + Off;
                    break;
                }
            }
        }
        free(Buf);
        if (Cave) break;
    }

    if (!Cave)
    {
        CloseHandle(hPort);
        return false;
    }

    ThreadPoolInject Tpd = {};
    Tpd.Callback = TramAddr;

    NTSTATUS st = Nt.WriteVirtualMemory(hProcess, Cave, &Tpd, sizeof(Tpd), nullptr);
    if (!NT_SUCCESS(st))
    {
        CloseHandle(hPort);
        return false;
    }

    st = Nt.SetIoCompletion(hPort, Cave, nullptr, 0, 0);
    CloseHandle(hPort);

    return NT_SUCCESS(st);
}
