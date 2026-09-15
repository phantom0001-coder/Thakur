#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <windows.h>

namespace Voltro {

// Inter-process communication commands passing between frontend UI and execution engine
enum class IpcCommandType {
    ATTACH_REQUEST,
    DETACH_REQUEST,
    EXECUTE_PAYLOAD,
    STOP_EXECUTION,
    CONFIG_SAVE,
    HEARTBEAT_PING
};

struct IpcMessage {
    std::string type;
    std::string script;
    int tab_id = 0;
    std::string plan;
    std::string version = "1.5.0";
};

class NamedPipeBridge {
public:
    static NamedPipeBridge& getInstance();

    bool startServer(const std::wstring& pipeName = L"\\\\.\\pipe\\VoltroIPC");
    void stopServer();
    bool isConnected() const;

    bool sendLogMessage(const std::string& level, const std::string& message);
    bool sendScriptPayload(const std::string& script);
    void setPayloadHandler(std::function<void(const IpcMessage&)> handler);

private:
    NamedPipeBridge();
    ~NamedPipeBridge();

    HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
    bool m_running = false;
    bool m_clientConnected = false;
    std::thread m_listenerThread;
    std::function<void(const IpcMessage&)> m_payloadHandler;
};

} // namespace Voltro
