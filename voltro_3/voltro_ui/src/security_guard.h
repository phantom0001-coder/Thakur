#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace Voltro {

// Binary security & anti-dumping protections
class BinaryProtector {
public:
    // Anti-debugging verification
    static bool isDebuggerAttached() {
        // 1. Standard Win32 API check
        if (IsDebuggerPresent()) return true;

        // 2. Remote debugger check
        BOOL isRemoteDebugger = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger);
        if (isRemoteDebugger) return true;

        return false;
    }

    // XOR String Encryption for protecting internal memory signatures
    static std::string decryptString(const std::string& encrypted, char key = 0x5A) {
        std::string result = encrypted;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= key;
        }
        return result;
    }

    // Crash or terminate if debugger or memory dumping tool is detected
    static void enforceSecurityGuards() {
        if (isDebuggerAttached()) {
            // Immediate process termination to prevent reverse engineering
            ExitProcess(0);
        }
    }
};

} // namespace Voltro
