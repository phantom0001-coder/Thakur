

namespace Thakur {

class BinaryProtector {
public:
    
    static bool isDebuggerAttached() {
        
        if (IsDebuggerPresent()) return true;

        
        BOOL isRemoteDebugger = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger);
        if (isRemoteDebugger) return true;

        return false;
    }

    
    static std::string decryptString(const std::string& encrypted, char key = 0x5A) {
        std::string result = encrypted;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= key;
        }
        return result;
    }

    
    static void enforceSecurityGuards() {
        if (isDebuggerAttached()) {
            
            ExitProcess(0);
        }
    }
};

} 
