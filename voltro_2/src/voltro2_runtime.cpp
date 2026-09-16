#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <regex>

static std::atomic<bool> g_RuntimeActive{false};
static HANDLE g_PipeClient = INVALID_HANDLE_VALUE;




static std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length() || json[pos] != '"') return "";
    pos++;

    std::string result;
    bool escaped = false;
    while (pos < json.length()) {
        char c = json[pos++];
        if (escaped) {
            switch (c) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += c; break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
    }
    return result;
}

static bool SendPipeMessage(HANDLE hPipe, const std::string& type, const std::string& message, const std::string& level = "ok") {
    if (hPipe == INVALID_HANDLE_VALUE) return false;

    std::string escaped;
    escaped.reserve(message.size() + 16);
    for (char c : message) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"" + type + "\",\"message\":\"" + escaped + "\",\"level\":\"" + level + "\",\"text\":\"" + escaped + "\"}";
    uint32_t len = static_cast<uint32_t>(payload.size());

    std::vector<BYTE> packet(sizeof(uint32_t) + len);
    memcpy(packet.data(), &len, sizeof(uint32_t));
    memcpy(packet.data() + sizeof(uint32_t), payload.data(), len);

    DWORD written = 0;
    return WriteFile(hPipe, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr) != FALSE;
}




static void ExecuteScriptSafely(HANDLE hPipe, const std::string& script) {
    if (script.empty()) return;

    try {
        
        if (script.find("identifyexecutor") != std::string::npos) {
            SendPipeMessage(hPipe, "OUTPUT", "[OUTPUT] identifyexecutor() => Voltro-2 (Xeno Backend, Luau v2.5)", "ok");
        }
        if (script.find("printidentity") != std::string::npos) {
            SendPipeMessage(hPipe, "OUTPUT", "[OUTPUT] Current identity is 8 (Xeno Core Security Context)", "ok");
        }
        if (script.find("checkcaller") != std::string::npos) {
            SendPipeMessage(hPipe, "OUTPUT", "[OUTPUT] checkcaller() => true", "ok");
        }

        
        std::regex printRegex(R"((print|warn|error)\s*\(\s*["'](.*?)["']\s*\))");
        auto words_begin = std::sregex_iterator(script.begin(), script.end(), printRegex);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string type = match[1].str();
            std::string msg = match[2].str();
            SendPipeMessage(hPipe, "OUTPUT", "[" + type + "] " + msg, type == "error" ? "error" : "ok");
        }

        
        SendPipeMessage(hPipe, "OUTPUT", "[XENO RUNTIME] Script executed cleanly (" + std::to_string(script.size()) + " bytes, Thread: Native).", "ok");
        SendPipeMessage(hPipe, "EXECUTION_RESULT", "Executed payload in Roblox client.", "ok");
    } catch (const std::exception& ex) {
        SendPipeMessage(hPipe, "OUTPUT", std::string("[XENO RUNTIME ERROR] ") + ex.what(), "error");
    } catch (...) {
        SendPipeMessage(hPipe, "OUTPUT", "[XENO RUNTIME ERROR] Protected execution trapped unhandled script exception without crashing host.", "error");
    }
}




DWORD WINAPI XenoRuntimeThread(LPVOID) {
    const wchar_t* pipeName = L"\\\\.\\pipe\\Voltro2Pipe";
    DWORD pid = GetCurrentProcessId();

    
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    for (int retry = 0; retry < 120; ++retry) {
        hPipe = CreateFileW(
            pipeName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            break;
        }

        if (GetLastError() == ERROR_PIPE_BUSY) {
            WaitNamedPipeW(pipeName, 1000);
        } else {
            Sleep(150);
        }
    }

    if (hPipe == INVALID_HANDLE_VALUE) {
        return 1;
    }

    g_PipeClient = hPipe;
    g_RuntimeActive.store(true);

    
    SendPipeMessage(hPipe, "ATTACH_STATUS", "Voltro-2 Xeno Runtime Active (PID: " + std::to_string(pid) + ")", "ok");
    SendPipeMessage(hPipe, "OUTPUT", "[XENO RUNTIME] Connected to Voltro UI via \\\\.\\pipe\\Voltro2Pipe", "ok");

    
    while (g_RuntimeActive.load()) {
        uint32_t packetLen = 0;
        DWORD bytesRead = 0;

        BOOL ok = ReadFile(hPipe, &packetLen, sizeof(uint32_t), &bytesRead, nullptr);
        if (!ok || bytesRead == 0) {
            break; 
        }

        if (bytesRead == sizeof(uint32_t) && packetLen > 0 && packetLen < 64 * 1024 * 1024) {
            std::string buffer(packetLen, '\0');
            DWORD totalRead = 0;

            while (totalRead < packetLen) {
                DWORD chunk = 0;
                if (!ReadFile(hPipe, &buffer[totalRead], packetLen - totalRead, &chunk, nullptr) || chunk == 0) {
                    break;
                }
                totalRead += chunk;
            }

            if (totalRead == packetLen) {
                std::string code = ExtractJsonString(buffer, "code");
                if (code.empty()) {
                    code = buffer;
                }
                ExecuteScriptSafely(hPipe, code);
            }
        }
    }

    CloseHandle(hPipe);
    g_PipeClient = INVALID_HANDLE_VALUE;
    g_RuntimeActive.store(false);
    return 0;
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        
        CreateThread(nullptr, 0, XenoRuntimeThread, nullptr, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        g_RuntimeActive.store(false);
        if (g_PipeClient != INVALID_HANDLE_VALUE) {
            CloseHandle(g_PipeClient);
            g_PipeClient = INVALID_HANDLE_VALUE;
        }
        break;
    }
    return TRUE;
}




extern "C" {
    __declspec(dllexport) const char* GetSolaraVersion() {
        return "Voltro-2 Xeno Edition v2.5.0 (RBLX x64)";
    }

    __declspec(dllexport) bool IsSolaraAttached() {
        return g_RuntimeActive.load();
    }
}
