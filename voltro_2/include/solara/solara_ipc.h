#pragma once

#ifndef SOLARA_IPC_H
#define SOLARA_IPC_H

#include "solara.h"
#include <thread>

namespace Solara {
namespace IPC {






class SolaraIpcBridge {
public:
    static SolaraIpcBridge& instance();

    bool start();
    void stop();

    bool isConnected() const { return m_connected.load(); }

    bool sendOutput(const std::string& text, const std::string& level = "ok");
    bool sendStatus(bool attached, const std::string& message = "");
    bool sendLog(const std::string& level, const std::string& message);

    void setScriptCallback(std::function<void(const std::string&)> callback) {
        m_scriptCallback = callback;
    }

private:
    SolaraIpcBridge() = default;
    ~SolaraIpcBridge();

    void clientWorkerLoop();
    void serverFallbackLoop();

    bool sendPacket(HANDLE pipe, const std::string& payload);
    bool readExact(HANDLE pipe, void* buffer, DWORD bytesToRead);

    std::string extractScriptCode(const std::string& rawPayload);

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    HANDLE m_clientPipe = INVALID_HANDLE_VALUE;
    HANDLE m_serverPipe = INVALID_HANDLE_VALUE;

    std::thread m_clientThread;
    std::thread m_serverThread;

    std::function<void(const std::string&)> m_scriptCallback;
};

} 
} 

#endif 
