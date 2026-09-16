#include "../include/solara/solara_ipc.h"
#include "../include/solara/solara_engine.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace Solara {
namespace IPC {

SolaraIpcBridge& SolaraIpcBridge::instance() {
    static SolaraIpcBridge inst;
    return inst;
}

SolaraIpcBridge::~SolaraIpcBridge() {
    stop();
}

bool SolaraIpcBridge::readExact(HANDLE pipe, void* buffer, DWORD bytesToRead) {
    DWORD totalRead = 0;
    BYTE* ptr = reinterpret_cast<BYTE*>(buffer);

    while (totalRead < bytesToRead) {
        DWORD read = 0;
        BOOL ok = ReadFile(pipe, ptr + totalRead, bytesToRead - totalRead, &read, nullptr);
        if (!ok || read == 0) {
            return false;
        }
        totalRead += read;
    }
    return true;
}

bool SolaraIpcBridge::sendPacket(HANDLE pipe, const std::string& payload) {
    if (pipe == INVALID_HANDLE_VALUE) return false;

    uint32_t length = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> packet(sizeof(uint32_t) + length);
    memcpy(packet.data(), &length, sizeof(uint32_t));
    memcpy(packet.data() + sizeof(uint32_t), payload.data(), length);

    DWORD written = 0;
    return WriteFile(pipe, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr) != FALSE;
}

std::string SolaraIpcBridge::extractScriptCode(const std::string& rawPayload) {
    
    size_t codeKey = rawPayload.find("\"code\"");
    if (codeKey != std::string::npos) {
        size_t colon = rawPayload.find(':', codeKey);
        if (colon != std::string::npos) {
            size_t startQuote = rawPayload.find('"', colon);
            if (startQuote != std::string::npos) {
                std::string code;
                code.reserve(rawPayload.size());
                bool escaped = false;
                for (size_t i = startQuote + 1; i < rawPayload.size(); ++i) {
                    char c = rawPayload[i];
                    if (escaped) {
                        if (c == 'n') code += '\n';
                        else if (c == 'r') code += '\r';
                        else if (c == 't') code += '\t';
                        else if (c == '"') code += '"';
                        else if (c == '\\') code += '\\';
                        else code += c;
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        break; 
                    } else {
                        code += c;
                    }
                }
                return code;
            }
        }
    }

    
    return rawPayload;
}

bool SolaraIpcBridge::sendOutput(const std::string& text, const std::string& level) {
    std::string escaped;
    escaped.reserve(text.size() + 32);
    for (char c : text) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"OUTPUT\",\"text\":\"" + escaped + "\",\"level\":\"" + level + "\"}";
    return sendPacket(m_clientPipe, payload);
}

bool SolaraIpcBridge::sendStatus(bool attached, const std::string& message) {
    std::string payload = "{\"type\":\"ATTACH_STATUS\",\"attached\":" +
                          std::string(attached ? "true" : "false") +
                          ",\"message\":\"" + message + "\"}";
    return sendPacket(m_clientPipe, payload);
}

bool SolaraIpcBridge::sendLog(const std::string& level, const std::string& message) {
    std::string escaped;
    escaped.reserve(message.size() + 32);
    for (char c : message) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"log_stream\",\"level\":\"" + level + "\",\"message\":\"" + escaped + "\"}";
    return sendPacket(m_clientPipe, payload);
}

void SolaraIpcBridge::clientWorkerLoop() {
    const wchar_t* targetPipe = L"\\\\.\\pipe\\VoltroIPC";

    while (m_running.load()) {
        m_clientPipe = CreateFileW(
            targetPipe,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (m_clientPipe == INVALID_HANDLE_VALUE) {
            m_connected.store(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }

        m_connected.store(true);
        sendStatus(true, "Voltro 2 (Solara RBLX Backend) attached.");
        sendLog("ok", "[SOLARA IPC] Connected to Voltro UI pipe (\\\\.\\pipe\\VoltroIPC).");

        while (m_running.load() && m_connected.load()) {
            uint32_t packetLen = 0;
            if (!readExact(m_clientPipe, &packetLen, sizeof(uint32_t))) {
                break; 
            }

            if (packetLen == 0 || packetLen > 64 * 1024 * 1024) {
                break; 
            }

            std::string buffer(packetLen, '\0');
            if (!readExact(m_clientPipe, &buffer[0], packetLen)) {
                break;
            }

            std::string script = extractScriptCode(buffer);
            if (m_scriptCallback) {
                m_scriptCallback(script);
            } else {
                Engine::SolaraEngine::instance().queueScript(script);
            }
        }

        m_connected.store(false);
        if (m_clientPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_clientPipe);
            m_clientPipe = INVALID_HANDLE_VALUE;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void SolaraIpcBridge::serverFallbackLoop() {
    const wchar_t* fallbackPipe = L"\\\\.\\pipe\\Voltro2IPC";

    while (m_running.load()) {
        m_serverPipe = CreateNamedPipeW(
            fallbackPipe,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536,
            65536,
            0,
            nullptr
        );

        if (m_serverPipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        BOOL clientConnected = ConnectNamedPipe(m_serverPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (clientConnected) {
            while (m_running.load()) {
                uint32_t len = 0;
                if (!readExact(m_serverPipe, &len, sizeof(len))) break;
                if (len == 0 || len > 64 * 1024 * 1024) break;

                std::string buf(len, '\0');
                if (!readExact(m_serverPipe, &buf[0], len)) break;

                std::string script = extractScriptCode(buf);
                Engine::SolaraEngine::instance().queueScript(script);
            }
            DisconnectNamedPipe(m_serverPipe);
        }

        CloseHandle(m_serverPipe);
        m_serverPipe = INVALID_HANDLE_VALUE;
    }
}

bool SolaraIpcBridge::start() {
    if (m_running.exchange(true)) return true;

    m_clientThread = std::thread(&SolaraIpcBridge::clientWorkerLoop, this);
    m_serverThread = std::thread(&SolaraIpcBridge::serverFallbackLoop, this);
    return true;
}

void SolaraIpcBridge::stop() {
    m_running.store(false);
    m_connected.store(false);

    if (m_clientPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_clientPipe);
        m_clientPipe = INVALID_HANDLE_VALUE;
    }

    if (m_serverPipe != INVALID_HANDLE_VALUE) {
        
        HANDLE dummy = CreateFileW(L"\\\\.\\pipe\\Voltro2IPC", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (dummy != INVALID_HANDLE_VALUE) CloseHandle(dummy);
        CloseHandle(m_serverPipe);
        m_serverPipe = INVALID_HANDLE_VALUE;
    }

    if (m_clientThread.joinable()) m_clientThread.detach();
    if (m_serverThread.joinable()) m_serverThread.detach();
}

} 
} 




extern "C" {
    __declspec(dllexport) void StartPipeServer() {
        Solara::IPC::SolaraIpcBridge::instance().start();
    }

    __declspec(dllexport) void StopPipeServer() {
        Solara::IPC::SolaraIpcBridge::instance().stop();
    }
}
