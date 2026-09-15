#pragma once

#ifndef SOLARA_CORE_H
#define SOLARA_CORE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

namespace Solara {

// -----------------------------------------------------------------------------
// Solara Architecture: Luau VM & Identity Constants (Roblox)
// -----------------------------------------------------------------------------

typedef void* lua_State_t;

// Capability bitmask enabling all script capabilities (equivalent to Identity 8)
constexpr uint64_t CAPABILITIES_ALL = 0x3FFFFFFFFFFFFFFFULL;
constexpr int IDENTITY_MAX = 8;

#pragma pack(push, 1)
struct RobloxExtraSpace {
    uint8_t pad_00[0x18];
    uint64_t capabilities;
    uint8_t pad_20[0x20];
    int identity;
};
#pragma pack(pop)

// -----------------------------------------------------------------------------
// Memory Safety & SEH Utilities
// -----------------------------------------------------------------------------
inline bool IsMemoryReadable(LPCVOID address, size_t size) {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(address, &mbi, sizeof(mbi))) {
        return false;
    }
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;
    return (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
}

} // namespace Solara

#endif // SOLARA_CORE_H
