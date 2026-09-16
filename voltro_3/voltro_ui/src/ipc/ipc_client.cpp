#include "ipc_client.h"
#include <iostream>
#include <sstream>

namespace Voltro {

NamedPipeBridge& NamedPipeBridge::getInstance() {
    static NamedPipeBridge instance;
    return instance;
}

NamedPipeBridge::NamedPipeBridge() : m_pipeHandle(INVALID_HANDLE_VALUE), m_running(false), m_clientConnected(false) {}

NamedPipeBridge::~NamedPipeBridge() {
    stopServer();
}

bool NamedPipeBridge::startServer(const std::wstring& pipeName) {
    if (m_running) return true;

    m_pipeHandle = CreateNamedPipeW(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        65536,
        65536,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );

    if (m_pipeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    m_running = true;
    m_clientConnected = false;

    m_listenerThread = std::thread([this]() {
        while (m_running) {
            BOOL connected = ConnectNamedPipe(m_pipeHandle, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (connected) {
                m_clientConnected = true;

                while (m_running && m_clientConnected) {
                    
                    uint32_t packetLen = 0;
                    DWORD bytesRead = 0;
                    BOOL ok = ReadFile(m_pipeHandle, &packetLen, sizeof(uint32_t), &bytesRead, NULL);
                    if (!ok || bytesRead == 0) {
                        break;
                    }

                    if (bytesRead == sizeof(uint32_t) && packetLen > 0 && packetLen < 64 * 1024 * 1024) {
                        std::string packetData(packetLen, '\0');
                        DWORD totalRead = 0;
                        while (totalRead < packetLen) {
                            DWORD readChunk = 0;
                            if (!ReadFile(m_pipeHandle, &packetData[totalRead], packetLen - totalRead, &readChunk, NULL) || readChunk == 0) {
                                break;
                            }
                            totalRead += readChunk;
                        }

                        if (totalRead == packetLen && m_payloadHandler) {
                            IpcMessage msg;
                            msg.type = "response";
                            msg.script = packetData;
                            m_payloadHandler(msg);
                        }
                    } else if (bytesRead > 0) {
                        
                        char rawBuf[2048];
                        memcpy(rawBuf, &packetLen, bytesRead);
                        DWORD extraRead = 0;
                        ReadFile(m_pipeHandle, rawBuf + bytesRead, sizeof(rawBuf) - 1 - bytesRead, &extraRead, NULL);
                        DWORD totalRaw = bytesRead + extraRead;
                        rawBuf[totalRaw] = '\0';
                        if (m_payloadHandler) {
                            IpcMessage msg;
                            msg.type = "response";
                            msg.script = std::string(rawBuf, totalRaw);
                            m_payloadHandler(msg);
                        }
                    }
                }

                m_clientConnected = false;
                DisconnectNamedPipe(m_pipeHandle);
            } else {
                Sleep(200);
            }
        }
    });

    return true;
}

void NamedPipeBridge::stopServer() {
    m_running = false;
    m_clientConnected = false;
    if (m_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
    if (m_listenerThread.joinable()) {
        m_listenerThread.detach();
    }
}

bool NamedPipeBridge::isConnected() const {
    return m_clientConnected;
}

bool NamedPipeBridge::sendLogMessage(const std::string& level, const std::string& message) {
    if (!m_running || m_pipeHandle == INVALID_HANDLE_VALUE || !m_clientConnected) return false;

    std::ostringstream ss;
    ss << "{\"type\":\"log_stream\",\"level\":\"" << level << "\",\"message\":\"" << message << "\"}\n";
    std::string payload = ss.str();

    uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> framedPacket(sizeof(uint32_t) + len);
    memcpy(framedPacket.data(), &len, sizeof(uint32_t));
    memcpy(framedPacket.data() + sizeof(uint32_t), payload.data(), len);

    DWORD bytesWritten = 0;
    return WriteFile(m_pipeHandle, framedPacket.data(), static_cast<DWORD>(framedPacket.size()), &bytesWritten, NULL) != FALSE;
}

bool NamedPipeBridge::sendScriptPayload(const std::string& script) {
    if (!m_running || m_pipeHandle == INVALID_HANDLE_VALUE || !m_clientConnected) return false;

    
    std::string escaped;
    escaped.reserve(script.size() + 32);
    for (char c : script) {
        if (c == '"') escaped += "\\\"";
        else if (c == '\\') escaped += "\\\\";
        else if (c == '\n') escaped += "\\n";
        else if (c == '\r') escaped += "\\r";
        else if (c == '\t') escaped += "\\t";
        else escaped += c;
    }

    std::string payload = "{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"" + escaped + "\"}";

    
    uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> framedPacket(sizeof(uint32_t) + len);
    memcpy(framedPacket.data(), &len, sizeof(uint32_t));
    memcpy(framedPacket.data() + sizeof(uint32_t), payload.data(), len);

    DWORD bytesWritten = 0;
    return WriteFile(m_pipeHandle, framedPacket.data(), static_cast<DWORD>(framedPacket.size()), &bytesWritten, NULL) != FALSE;
}

void NamedPipeBridge::setPayloadHandler(std::function<void(const IpcMessage&)> handler) {
    m_payloadHandler = handler;
}

} 
