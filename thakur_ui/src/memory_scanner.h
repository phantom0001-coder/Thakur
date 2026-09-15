

namespace Thakur {

class MemoryScanner {
public:
    
    static uintptr_t scanPattern(HANDLE hProcess, uintptr_t baseAddress, size_t regionSize, const std::string& pattern, const std::string& mask) {
        if (!hProcess || pattern.empty() || mask.empty() || pattern.size() != mask.size()) {
            return 0;
        }

        std::vector<uint8_t> buffer(regionSize);
        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(baseAddress), buffer.data(), regionSize, &bytesRead) || bytesRead == 0) {
            return 0;
        }

        size_t patternLen = pattern.size();
        for (size_t i = 0; i <= bytesRead - patternLen; ++i) {
            bool matched = true;
            for (size_t j = 0; j < patternLen; ++j) {
                if (mask[j] != '?' && static_cast<uint8_t>(pattern[j]) != buffer[i + j]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return baseAddress + i;
            }
        }

        return 0;
    }
};

} 
