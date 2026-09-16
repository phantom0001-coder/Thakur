#pragma once
#include <cstdint>

// Community-sourced Roblox offsets (version-c5aecda2245e4fae)
// Source: Roblox-Offsets-Website
// These are RVA offsets from the Roblox module base address.
namespace RobloxOffsets {

    // --- Engine Function RVAs ---
    namespace Engine {
        inline constexpr std::uintptr_t LuaVM_Load            = 0x41B3B50;
        inline constexpr std::uintptr_t TaskSchedulerConstructor = 0x2AD5F50;
        inline constexpr std::uintptr_t TaskSchedulerDumpJobs = 0x2AD8DC0;
        inline constexpr std::uintptr_t TaskSchedulerAddJob   = 0x2ADB000;
        inline constexpr std::uintptr_t Print                 = 0x1CAB4B0;
        inline constexpr std::uintptr_t GetTlsPointer         = 0x4250;
        inline constexpr std::uintptr_t FreeString            = 0x7D6F80;
    }

    // --- Game Function RVAs ---
    namespace Game {
        inline constexpr std::uintptr_t PrintIdentity          = 0x4267C80;
        inline constexpr std::uintptr_t ScriptContextResume    = 0x4260F50;
        inline constexpr std::uintptr_t ScriptContextGetLuauSecurityFlags = 0x41BD670;
        inline constexpr std::uintptr_t InstanceFindFirstChild = 0x1642D80;
        inline constexpr std::uintptr_t DataModelGetService    = 0x296DB10;
    }

    // --- Global Pointers (RVA of pointers that hold object addresses) ---
    namespace Pointer {
        inline constexpr std::uintptr_t TaskScheduler          = 0x7B68F00;
        inline constexpr std::uintptr_t VisualEnginePointer    = 0x763D3D0;
        inline constexpr std::uintptr_t FakeDataModelPointer   = 0x7A8F908;
    }

    // --- TaskScheduler internal offsets ---
    namespace TaskScheduler {
        inline constexpr std::uintptr_t JobStart               = 0xC8;
        inline constexpr std::uintptr_t JobEnd                 = 0xD0;
        inline constexpr std::uintptr_t MaxFPS                 = 0xB0;
    }

    // --- ScriptContext / Lua State ---
    namespace ScriptContext {
        inline constexpr std::uintptr_t LuauSecurityFlags      = 0x41BD670;
    }

    // --- Instance offsets ---
    namespace Instance {
        inline constexpr std::uintptr_t Name                   = 0x70;
        inline constexpr std::uintptr_t Parent                 = 0x68;
        inline constexpr std::uintptr_t Children               = 0x78;
        inline constexpr std::uintptr_t ClassDescriptor        = 0x18;
    }

    // --- DataModel offsets ---
    namespace DataModel {
        inline constexpr std::uintptr_t ScriptContext           = 0x3F0;
        inline constexpr std::uintptr_t Workspace              = 0x178;
    }
}
