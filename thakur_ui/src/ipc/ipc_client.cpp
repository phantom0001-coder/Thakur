#include <windows.h>
#include <string>
#include <thread>
#include <functional>
#include <vector>
#include <mutex>
#include <sstream>
#include <cstring>
#include <algorithm>
#include "ipc_client.h"

namespace Thakur {

namespace {
std::mutex g_pipeStateMutex;

bool isValidPipe(HANDLE handle) {
    return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}

std::string escapeJson(const std::string& value) {
    std::ostringstream ss;
    for (unsigned char c : value) {
        switch (c) {
        case '"': ss << "\\\""; break;
        case '\\': ss << "\\\\"; break;
        case '\b': ss << "\\b"; break;
        case '\f': ss << "\\f"; break;
        case '\n': ss << "\\n"; break;
        case '\r': ss << "\\r"; break;
        case '\t': ss << "\\t"; break;
        default:
            if (c < 0x20) {
                char hex[7];
                sprintf_s(hex, "\\u%04x", c);
                ss << hex;
            } else {
                ss << static_cast<char>(c);
            }
        }
    }
    return ss.str();
}

bool readAvailable(HANDLE pipe, void* buffer, DWORD length) {
    if (!isValidPipe(pipe) || length == 0) return false;

    DWORD totalRead = 0;
    auto* out = static_cast<BYTE*>(buffer);

    while (totalRead < length) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) {
            return false;
        }

        if (available == 0) {
            Sleep(20);
            continue;
        }

        const DWORD toRead = (std::min)(available, length - totalRead);
        DWORD chunkRead = 0;
        if (!ReadFile(pipe, out + totalRead, toRead, &chunkRead, nullptr) || chunkRead == 0) {
            return false;
        }
        totalRead += chunkRead;
    }

    return true;
}
}

NamedPipeBridge& NamedPipeBridge::getInstance() {
    static NamedPipeBridge instance;
    return instance;
}

NamedPipeBridge::NamedPipeBridge()
    : m_pipeHandle(INVALID_HANDLE_VALUE),
      m_running(false),
      m_clientConnected(false) {}

NamedPipeBridge::~NamedPipeBridge() {
    stopServer();
}

bool NamedPipeBridge::startServer(const std::wstring& pipeName) {
    {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        if (m_running) return true;

        m_pipeHandle = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536,
            65536,
            0,
            nullptr
        );

        if (!isValidPipe(m_pipeHandle)) {
            m_pipeHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        m_running = true;
        m_clientConnected = false;
    }

    m_listenerThread = std::thread([this]() {
        while (true) {
            HANDLE pipe = INVALID_HANDLE_VALUE;
            {
                std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                if (!m_running || !isValidPipe(m_pipeHandle)) break;
                pipe = m_pipeHandle;
            }

            const BOOL connected = ConnectNamedPipe(pipe, nullptr)
                ? TRUE
                : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (!connected) {
                const DWORD error = GetLastError();
                if (error == ERROR_PIPE_LISTENING || error == ERROR_NO_DATA) {
                    Sleep(25);
                    continue;
                }

                std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                if (!m_running) break;
                Sleep(25);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                m_clientConnected = true;
            }

            while (true) {
                {
                    std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                    if (!m_running || !m_clientConnected) break;
                }

                DWORD available = 0;
                if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) {
                    break;
                }

                if (available < sizeof(uint32_t)) {
                    Sleep(20);
                    continue;
                }

                uint32_t packetLen = 0;
                if (!readAvailable(pipe, &packetLen, sizeof(packetLen))) {
                    break;
                }

                if (packetLen == 0 || packetLen >= 64 * 1024 * 1024) {
                    break;
                }

                std::string packetData(packetLen, '\0');
                if (!readAvailable(pipe, packetData.data(), packetLen)) {
                    break;
                }

                std::function<void(const IpcMessage&)> handler;
                {
                    std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                    handler = m_payloadHandler;
                }

                if (handler) {
                    IpcMessage msg;
                    msg.type = "response";
                    msg.script = std::move(packetData);
                    handler(msg);
                }
            }

            {
                std::lock_guard<std::mutex> lock(g_pipeStateMutex);
                m_clientConnected = false;
                if (m_running && isValidPipe(pipe)) {
                    DisconnectNamedPipe(pipe);
                }
            }

            if (!m_running) break;
        }
    });

    return true;
}

void NamedPipeBridge::stopServer() {
    {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        m_running = false;
        m_clientConnected = false;
        if (isValidPipe(m_pipeHandle)) {
            DisconnectNamedPipe(m_pipeHandle);
        }
    }

    if (m_listenerThread.joinable()) {
        m_listenerThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        if (isValidPipe(m_pipeHandle)) {
            CloseHandle(m_pipeHandle);
        }
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
}

bool NamedPipeBridge::isConnected() const {
    std::lock_guard<std::mutex> lock(g_pipeStateMutex);
    return m_clientConnected;
}

bool NamedPipeBridge::sendLogMessage(const std::string& level, const std::string& message) {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        if (!m_running || !isValidPipe(m_pipeHandle) || !m_clientConnected) return false;
        pipe = m_pipeHandle;
    }

    const std::string payload =
        "{\"type\":\"log_stream\",\"level\":\"" +
        escapeJson(level) +
        "\",\"message\":\"" +
        escapeJson(message) +
        "\"}";

    const uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> framedPacket(sizeof(len) + len);
    memcpy(framedPacket.data(), &len, sizeof(len));
    memcpy(framedPacket.data() + sizeof(len), payload.data(), len);

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe, framedPacket.data(), static_cast<DWORD>(framedPacket.size()), &bytesWritten, nullptr)) {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        m_clientConnected = false;
        return false;
    }

    return bytesWritten == framedPacket.size();
}

bool NamedPipeBridge::sendScriptPayload(const std::string& script) {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        if (!m_running || !isValidPipe(m_pipeHandle) || !m_clientConnected) return false;
        pipe = m_pipeHandle;
    }

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

    const std::string payload =
        "{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"" + escaped + "\"}";

    const uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<BYTE> framedPacket(sizeof(len) + len);
    memcpy(framedPacket.data(), &len, sizeof(len));
    memcpy(framedPacket.data() + sizeof(len), payload.data(), len);

    DWORD bytesWritten = 0;
    if (!WriteFile(pipe, framedPacket.data(), static_cast<DWORD>(framedPacket.size()), &bytesWritten, nullptr)) {
        std::lock_guard<std::mutex> lock(g_pipeStateMutex);
        m_clientConnected = false;
        return false;
    }

    return bytesWritten == framedPacket.size();
}

void NamedPipeBridge::setPayloadHandler(std::function<void(const IpcMessage&)> handler) {
    std::lock_guard<std::mutex> lock(g_pipeStateMutex);
    m_payloadHandler = std::move(handler);
}

} 
