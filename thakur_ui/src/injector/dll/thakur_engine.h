

namespace Thakur {
namespace Runtime {

typedef void* lua_State_t;

constexpr uint64_t CAPABILITIES_ALL = 0x3FFFFFFFFFFFFFFFULL;

struct RobloxExtraSpace {
    uint8_t pad_00[0x18];
    uint64_t capabilities;
    uint8_t pad_20[0x20];
    int identity;
};

class ThakurExecutionEngine {
public:
    static ThakurExecutionEngine& instance();

    bool initialize();
    void shutdown();

    
    bool queueScript(const std::string& script);

    
    bool isHooked() const { return m_hooked.load(); }
    lua_State_t getLuaState() const { return m_luaState; }

    
    typedef void(__fastcall* JobStepFn)(void* thisPtr, void* stats);
    void onJobStep();

private:
    ThakurExecutionEngine() = default;
    ~ThakurExecutionEngine();

    uintptr_t findTaskScheduler();
    uintptr_t scanTaskSchedulerWorker();
    bool hookSchedulerJob(uintptr_t scheduler);
    void setupLuauEnvironment(lua_State_t L);

    std::atomic<bool> m_hooked{ false };
    std::atomic<bool> m_running{ false };
    lua_State_t m_luaState = nullptr;
    uintptr_t m_scriptContext = 0;
    uintptr_t m_targetJob = 0;

    
    JobStepFn m_originalJobStep = nullptr;
    uintptr_t* m_vftable = nullptr;

    std::queue<std::string> m_scriptQueue;
    std::mutex m_queueMutex;
};

class FramedIpcBridge {
public:
    static FramedIpcBridge& instance();

    bool start();
    void stop();

    bool sendOutput(const std::string& text, const std::string& level = "print");
    bool sendStatus(bool attached, const std::string& message = "");

    void setScriptHandler(std::function<void(const std::string&)> handler);

private:
    FramedIpcBridge() = default;
    ~FramedIpcBridge();

    void communicationLoop();
    bool sendPacket(const std::string& payload);
    bool readExact(HANDLE pipe, void* buffer, DWORD bytesToRead);

    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::atomic<bool> m_running{ false };
    std::thread m_workerThread;
    std::function<void(const std::string&)> m_scriptHandler;
};

} 
} 
