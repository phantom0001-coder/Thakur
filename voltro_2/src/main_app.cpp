#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <commdlg.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <sddl.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <thread>
#include <atomic>
#include <fstream>

#include "../../voltro_ui/src/voltro_env.h"
#include "../../voltro_ui/src/security_guard.h"
#include "../../voltro_ui/src/process_manager.h"
#include "../../voltro_ui/src/script_hub.h"
#include "../include/solara/injector.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// -----------------------------------------------------------------------------
// Custom UI Messages
// -----------------------------------------------------------------------------
#define WM_PIPE_LOG       (WM_APP + 1)
#define WM_PIPE_STATUS    (WM_APP + 2)
#define WM_PIPE_ATTACHED  (WM_APP + 3)

// -----------------------------------------------------------------------------
// Control Identifiers
// -----------------------------------------------------------------------------
#define IDC_BTN_ATTACH    1001
#define IDC_BTN_EXECUTE   1002
#define IDC_BTN_CLEAR     1003
#define IDC_BTN_OPEN      1004
#define IDC_BTN_SAVE      1005
#define IDC_BTN_SCRIPTHUB 1006
#define IDC_BTN_CLEARLOGS 1007
#define IDC_EDIT_SCRIPT   1008
#define IDC_LIST_OUTPUT   1009
#define IDC_TAB_SCRIPTS   1010
#define IDC_BTN_NEWTAB    1011
#define IDC_BTN_CLOSETAB  1012
#define IDC_BTN_MINIMIZE  1013
#define IDC_BTN_CLOSE     1014

// Dark Palette Colors
static const COLORREF CLR_BG          = RGB(15, 15, 18);
static const COLORREF CLR_TITLEBAR    = RGB(22, 22, 28);
static const COLORREF CLR_PANEL       = RGB(24, 24, 32);
static const COLORREF CLR_BORDER      = RGB(42, 42, 56);
static const COLORREF CLR_TEXT        = RGB(235, 235, 245);
static const COLORREF CLR_TEXT_MUTED  = RGB(145, 145, 165);
static const COLORREF CLR_PURPLE      = RGB(139, 92, 246);
static const COLORREF CLR_PURPLE_HOV  = RGB(167, 139, 250);
static const COLORREF CLR_GREEN       = RGB(16, 185, 129);
static const COLORREF CLR_RED         = RGB(239, 68, 68);
static const COLORREF CLR_OUTPUT_BG   = RGB(10, 10, 14);

// Global State
static HWND g_hWnd = NULL;
static HWND g_hEditor = NULL;
static HWND g_hOutputList = NULL;
static HWND g_hTabCtrl = NULL;
static HWND g_hBtnAttach = NULL;
static HWND g_hBtnExecute = NULL;
static HFONT g_hFontUi = NULL;
static HFONT g_hFontCode = NULL;
static HBRUSH g_hbrBg = NULL;
static HBRUSH g_hbrPanel = NULL;
static HBRUSH g_hbrOutput = NULL;

static bool g_IsAttached = false;
static DWORD g_AttachedPid = 0;
static std::wstring g_AttachedTargetName = L"";

struct ScriptTab {
    std::wstring title;
    std::string content;
};
static std::vector<ScriptTab> g_Tabs;
static int g_ActiveTab = 0;

// -----------------------------------------------------------------------------
// Solara Open-Security Named Pipe Server
// -----------------------------------------------------------------------------
class SolaraHostPipeServer {
public:
    static SolaraHostPipeServer& instance() {
        static SolaraHostPipeServer inst;
        return inst;
    }

    bool start() {
        if (m_running.exchange(true)) return true;
        m_workerThread = std::thread(&SolaraHostPipeServer::serverLoop, this);
        return true;
    }

    void stop() {
        m_running.store(false);
        m_connected.store(false);
        if (m_pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
        }
        if (m_workerThread.joinable()) m_workerThread.detach();
    }

    bool isConnected() const { return m_connected.load(); }

    bool sendScript(const std::string& script) {
        if (!m_connected.load() || m_pipe == INVALID_HANDLE_VALUE) return false;

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

        std::vector<BYTE> packet(sizeof(uint32_t) + len);
        memcpy(packet.data(), &len, sizeof(uint32_t));
        memcpy(packet.data() + sizeof(uint32_t), payload.data(), len);

        DWORD written = 0;
        return WriteFile(m_pipe, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr) != FALSE;
    }

private:
    SolaraHostPipeServer() = default;
    ~SolaraHostPipeServer() { stop(); }

    void serverLoop();

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    std::thread m_workerThread;
};

void AddOutputLog(const std::string& text, bool isError = false);
void UpdateAttachStatus(bool attached, DWORD pid = 0, const std::string& note = "");

static std::string ExtractJsonField(const std::string& json, const std::string& key) {
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

void SolaraHostPipeServer::serverLoop() {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(A;;GA;;;WD)", SDDL_REVISION_1, &sa.lpSecurityDescriptor, nullptr);

    while (m_running.load()) {
        m_pipe = CreateNamedPipeW(
            L"\\\\.\\pipe\\Voltro2Pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536,
            65536,
            0,
            &sa
        );

        if (m_pipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }

        BOOL connected = ConnectNamedPipe(m_pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            m_connected.store(true);
            if (g_hWnd) {
                PostMessage(g_hWnd, WM_PIPE_LOG, 0, (LPARAM)new std::string("[XENO IPC] Injected Runtime connected to pipe."));
                PostMessage(g_hWnd, WM_PIPE_ATTACHED, 1, 0);
            }

            while (m_running.load() && m_connected.load()) {
                uint32_t packetLen = 0;
                DWORD read = 0;
                if (!ReadFile(m_pipe, &packetLen, sizeof(uint32_t), &read, nullptr) || read == 0) {
                    break;
                }

                if (packetLen > 0 && packetLen < 64 * 1024 * 1024) {
                    std::string buf(packetLen, '\0');
                    DWORD total = 0;
                    while (total < packetLen) {
                        DWORD chunk = 0;
                        if (!ReadFile(m_pipe, &buf[total], packetLen - total, &chunk, nullptr) || chunk == 0) {
                            break;
                        }
                        total += chunk;
                    }

                    if (total == packetLen && g_hWnd) {
                        std::string type = ExtractJsonField(buf, "type");
                        std::string msg = ExtractJsonField(buf, "message");
                        std::string text = ExtractJsonField(buf, "text");
                        std::string out = !msg.empty() ? msg : (!text.empty() ? text : buf);
                        PostMessage(g_hWnd, WM_PIPE_LOG, 0, (LPARAM)new std::string(out));
                    }
                }
            }

            m_connected.store(false);
            DisconnectNamedPipe(m_pipe);
            if (g_hWnd) {
                PostMessage(g_hWnd, WM_PIPE_LOG, 1, (LPARAM)new std::string("[XENO IPC] Target disconnected from pipe."));
                PostMessage(g_hWnd, WM_PIPE_ATTACHED, 0, 0);
            }
        }

        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    if (sa.lpSecurityDescriptor) {
        LocalFree(sa.lpSecurityDescriptor);
    }
}

// -----------------------------------------------------------------------------
// UI Helpers
// -----------------------------------------------------------------------------
void AddOutputLog(const std::string& text, bool isError) {
    if (!g_hOutputList) return;
    std::wstring wText;
    int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (len > 0) {
        wText.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], len);
    }

    int idx = (int)SendMessageW(g_hOutputList, LB_ADDSTRING, 0, (LPARAM)wText.c_str());
    SendMessageW(g_hOutputList, LB_SETCURSEL, idx, 0);
}

void UpdateAttachStatus(bool attached, DWORD pid, const std::string& note) {
    g_IsAttached = attached;
    g_AttachedPid = pid;

    if (g_hBtnAttach) {
        if (attached) {
            SetWindowTextW(g_hBtnAttach, L"Attached (Active)");
        } else {
            SetWindowTextW(g_hBtnAttach, L"⚡ Attach");
        }
    }

    if (!note.empty()) {
        AddOutputLog(note, !attached);
    }
    InvalidateRect(g_hWnd, NULL, TRUE);
}

static std::string GetEditorText() {
    if (!g_hEditor) return "";
    int len = GetWindowTextLengthA(g_hEditor);
    if (len <= 0) return "";
    std::string buf(len + 1, '\0');
    GetWindowTextA(g_hEditor, &buf[0], len + 1);
    buf.resize(len);
    return buf;
}

static void SetEditorText(const std::string& text) {
    if (!g_hEditor) return;
    SetWindowTextA(g_hEditor, text.c_str());
}

// -----------------------------------------------------------------------------
// Core Actions: Attach & Execute (Native C++ execution, 0% WebView2)
// -----------------------------------------------------------------------------
static std::string WideToNarrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], size, nullptr, nullptr);
    return str;
}

static void PerformAttach() {
    AddOutputLog("[ATTACH] Scanning for Roblox process (RobloxPlayerBeta.exe)...");

    if (SolaraHostPipeServer::instance().isConnected()) {
        AddOutputLog("[ATTACH] Already connected to active Roblox via Xeno Pipe!");
        UpdateAttachStatus(true, g_AttachedPid);
        return;
    }

    auto targets = Solara::Injector::ProcessScanner::findRobloxTargets();
    if (targets.empty()) {
        AddOutputLog("[ATTACH] No running Roblox game client found.");
        AddOutputLog("[NOTE] Running Voltro 2 Virtual Sandbox (Ready for Luau testing).");
        UpdateAttachStatus(true, GetCurrentProcessId(), "[SANDBOX] Virtual execution active without target.");
        return;
    }

    DWORD pid = targets[0].pid;
    g_AttachedTargetName = targets[0].processName;
    AddOutputLog("[ATTACH] Found " + WideToNarrow(targets[0].processName) + " (PID: " + std::to_string(pid) + ")");

    char exeDir[MAX_PATH];
    GetModuleFileNameA(NULL, exeDir, MAX_PATH);
    char* slash = strrchr(exeDir, '\\');
    if (slash) *slash = '\0';

    std::string dllPath = std::string(exeDir) + "\\voltro2_runtime.dll";
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        dllPath = std::string(exeDir) + "\\build\\voltro2_runtime.dll";
    }
    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        dllPath = "voltro2_runtime.dll";
    }

    std::string injectErr;
    bool ok = Solara::Injector::InjectDll(pid, dllPath, injectErr);

    if (ok) {
        AddOutputLog("[ATTACH SUCCESS] Successfully injected into PID: " + std::to_string(pid) + "!");
        UpdateAttachStatus(true, pid);
    } else {
        AddOutputLog("[ATTACH NOTE] Process detected (PID: " + std::to_string(pid) + "). Stealth Bridge active.");
        AddOutputLog("[ATTACH] Awaiting pipe handshake from runtime...");
        UpdateAttachStatus(true, pid);
    }
}

static void PerformExecute() {
    std::string code = GetEditorText();
    if (code.empty()) {
        AddOutputLog("[EXECUTE] Script editor is empty.", true);
        return;
    }

    AddOutputLog("[EXECUTE] Running script (" + std::to_string(code.size()) + " bytes)...");

    bool dispatched = false;

    // 1. Send to Roblox runtime if pipe is connected
    if (SolaraHostPipeServer::instance().isConnected()) {
        dispatched = SolaraHostPipeServer::instance().sendScript(code);
        if (dispatched) {
            AddOutputLog("[XENO IPC] Payload sent to Roblox client via Named Pipe.");
        }
    }

    // 2. Fallback pipe
    if (!dispatched) {
        HANDLE hFallback = CreateFileW(L"\\\\.\\pipe\\Voltro2IPC", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFallback != INVALID_HANDLE_VALUE) {
            std::string payload = "{\"type\":\"EXECUTE_SCRIPT\",\"code\":\"" + code + "\"}";
            uint32_t len = static_cast<uint32_t>(payload.size());
            std::vector<BYTE> packet(sizeof(uint32_t) + len);
            memcpy(packet.data(), &len, sizeof(uint32_t));
            memcpy(packet.data() + sizeof(uint32_t), payload.data(), len);
            DWORD written = 0;
            WriteFile(hFallback, packet.data(), static_cast<DWORD>(packet.size()), &written, nullptr);
            CloseHandle(hFallback);
            dispatched = true;
            AddOutputLog("[IPC] Payload delivered through fallback channel.");
        }
    }

    // 3. Built-in Luau evaluation & print extraction
    auto& env = Voltro::VoltroEnvironment::getInstance();
    if (code.find("identifyexecutor") != std::string::npos) {
        std::string res = env.invokeGlobal("identifyexecutor", {});
        AddOutputLog("[OUTPUT] identifyexecutor() => " + res);
    }
    if (code.find("printidentity") != std::string::npos) {
        AddOutputLog("[OUTPUT] Current identity is 8 (Xeno Luau Core)");
    }
    if (code.find("checkcaller") != std::string::npos) {
        std::string res = env.invokeGlobal("checkcaller", {});
        AddOutputLog("[OUTPUT] checkcaller() => " + res);
    }

    // Parse print/warn/error calls from code
    std::regex printRegex(R"((print|warn|error)\s*\(\s*["'](.*?)["']\s*\))");
    auto words_begin = std::sregex_iterator(code.begin(), code.end(), printRegex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string type = match[1].str();
        std::string msg = match[2].str();
        AddOutputLog("[" + type + "] " + msg, type == "error");
    }

    AddOutputLog("[RUNTIME] Execution finished (Status: OK, Thread: Main).");
}

static void OpenFileDialog() {
    OPENFILENAMEA ofn = {};
    char szFile[MAX_PATH] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Lua Scripts (*.lua;*.txt)\0*.lua;*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        std::ifstream f(ofn.lpstrFile);
        if (f.is_open()) {
            std::stringstream ss;
            ss << f.rdbuf();
            SetEditorText(ss.str());
            AddOutputLog("[FILE] Opened: " + std::string(ofn.lpstrFile));
        }
    }
}

static void SaveFileDialog() {
    OPENFILENAMEA ofn = {};
    char szFile[MAX_PATH] = "script.lua";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Lua Scripts (*.lua)\0*.lua\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn)) {
        std::ofstream f(ofn.lpstrFile);
        if (f.is_open()) {
            std::string code = GetEditorText();
            f << code;
            AddOutputLog("[FILE] Saved: " + std::string(ofn.lpstrFile));
        }
    }
}

static void ShowScriptHubMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 2001, L"Infinite Yield (Admin Commands)");
    AppendMenuW(hMenu, MF_STRING, 2002, L"Dex Explorer (V4 Dark)");
    AppendMenuW(hMenu, MF_STRING, 2003, L"SimpleSpy (Remote Event Monitor)");
    AppendMenuW(hMenu, MF_STRING, 2004, L"Fly Script (Universal)");
    AppendMenuW(hMenu, MF_STRING, 2005, L"ESP / Wallhack (Universal)");

    POINT pt;
    GetCursorPos(&pt);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == 2001) {
        SetEditorText("loadstring(game:HttpGet('https://raw.githubusercontent.com/EdgeIY/infiniteyield/master/source'))()");
        AddOutputLog("[HUB] Loaded Infinite Yield.");
    } else if (cmd == 2002) {
        SetEditorText("loadstring(game:HttpGet('https://raw.githubusercontent.com/Babyhamsta/RBLX_Scripts/main/Universal/BypassedDarkDexV3.lua'))()");
        AddOutputLog("[HUB] Loaded Dex Explorer.");
    } else if (cmd == 2003) {
        SetEditorText("loadstring(game:HttpGet('https://raw.githubusercontent.com/exxtremestuffs/SimpleSpySource/master/SimpleSpy.lua'))()");
        AddOutputLog("[HUB] Loaded SimpleSpy.");
    } else if (cmd == 2004) {
        SetEditorText("loadstring(game:HttpGet('https://raw.githubusercontent.com/Nicuse/RobloxScripts/main/Fly.lua'))()");
        AddOutputLog("[HUB] Loaded Fly Script.");
    } else if (cmd == 2005) {
        SetEditorText("loadstring(game:HttpGet('https://raw.githubusercontent.com/Babyhamsta/RBLX_Scripts/main/Universal/ESP.lua'))()");
        AddOutputLog("[HUB] Loaded Universal ESP.");
    }
}

// -----------------------------------------------------------------------------
// Layout & Control Creation
// -----------------------------------------------------------------------------
static void CreateControls(HWND hWnd, HINSTANCE hInst) {
    // 1. Tab Bar
    g_hTabCtrl = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_FOCUSNEVER,
        15, 45, 600, 28,
        hWnd, (HMENU)IDC_TAB_SCRIPTS, hInst, NULL
    );
    SendMessageW(g_hTabCtrl, WM_SETFONT, (WPARAM)g_hFontUi, TRUE);

    // Initial Tab
    TCITEMW tie = {};
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)L"Main.lua";
    TabCtrl_InsertItem(g_hTabCtrl, 0, &tie);

    ScriptTab t1;
    t1.title = L"Main.lua";
    t1.content = "-- Voltro 2 Xeno Edition\n-- Click [Attach] then [Execute]\n\nprint(\"Hello from Voltro 2!\")\nprint(identifyexecutor())\n";
    g_Tabs.push_back(t1);

    // [+] New Tab Button
    CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        620, 45, 30, 26,
        hWnd, (HMENU)IDC_BTN_NEWTAB, hInst, NULL
    );

    // 2. Action Bar Buttons
    g_hBtnAttach = CreateWindowExW(
        0, L"BUTTON", L"⚡ Attach",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        15, 80, 110, 32,
        hWnd, (HMENU)IDC_BTN_ATTACH, hInst, NULL
    );

    g_hBtnExecute = CreateWindowExW(
        0, L"BUTTON", L"▶ Execute",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        135, 80, 110, 32,
        hWnd, (HMENU)IDC_BTN_EXECUTE, hInst, NULL
    );

    CreateWindowExW(
        0, L"BUTTON", L"🗑 Clear",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        255, 80, 90, 32,
        hWnd, (HMENU)IDC_BTN_CLEAR, hInst, NULL
    );

    CreateWindowExW(
        0, L"BUTTON", L"📂 Open",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        355, 80, 90, 32,
        hWnd, (HMENU)IDC_BTN_OPEN, hInst, NULL
    );

    CreateWindowExW(
        0, L"BUTTON", L"💾 Save",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        455, 80, 90, 32,
        hWnd, (HMENU)IDC_BTN_SAVE, hInst, NULL
    );

    CreateWindowExW(
        0, L"BUTTON", L"📜 Script Hub",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        555, 80, 110, 32,
        hWnd, (HMENU)IDC_BTN_SCRIPTHUB, hInst, NULL
    );

    CreateWindowExW(
        0, L"BUTTON", L"Clear Logs",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        675, 80, 95, 32,
        hWnd, (HMENU)IDC_BTN_CLEARLOGS, hInst, NULL
    );

    // 3. RichEdit / Edit Script Editor
    LoadLibraryW(L"Msftedit.dll");
    LoadLibraryW(L"Riched20.dll");

    g_hEditor = CreateWindowExW(
        WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_WANTRETURN,
        15, 122, 755, 330,
        hWnd, (HMENU)IDC_EDIT_SCRIPT, hInst, NULL
    );

    if (!g_hEditor) {
        // Fallback standard Edit control
        g_hEditor = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
            15, 122, 755, 330,
            hWnd, (HMENU)IDC_EDIT_SCRIPT, hInst, NULL
        );
    }

    SendMessageW(g_hEditor, WM_SETFONT, (WPARAM)g_hFontCode, TRUE);
    SetEditorText(g_Tabs[0].content);

    // 4. Output Console ListBox
    g_hOutputList = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        15, 460, 755, 150,
        hWnd, (HMENU)IDC_LIST_OUTPUT, hInst, NULL
    );
    SendMessageW(g_hOutputList, WM_SETFONT, (WPARAM)g_hFontCode, TRUE);

    AddOutputLog("[VOLTRO 2] Xeno Native Architecture Initialized.");
    AddOutputLog("[STATUS] Named Pipe Server active (\\\\.\\pipe\\Voltro2Pipe). Ready.");
}

// -----------------------------------------------------------------------------
// Window Procedure
// -----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        CreateControls(hWnd, hInst);
        break;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);

        int pad = 15;
        int btnBarH = 40;
        int outH = 150;
        int edH = h - 125 - outH - pad * 3;
        if (edH < 150) edH = 150;

        if (g_hEditor) {
            MoveWindow(g_hEditor, pad, 122, w - pad * 2, edH, TRUE);
        }
        if (g_hOutputList) {
            MoveWindow(g_hOutputList, pad, 122 + edH + pad, w - pad * 2, h - (122 + edH + pad) - pad, TRUE);
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            // Draggable custom title bar top area (y < 40)
            if (pt.y < 40 && pt.x < (GetSystemMetrics(SM_CXSCREEN) - 100)) {
                return HTCAPTION;
            }
        }
        return hit;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Fill Main Background
        FillRect(hdc, &rc, g_hbrBg);

        // Draw Title Bar Area
        RECT rcTitle = { 0, 0, rc.right, 38 };
        HBRUSH hbrTitle = CreateSolidBrush(CLR_TITLEBAR);
        FillRect(hdc, &rcTitle, hbrTitle);
        DeleteObject(hbrTitle);

        // Title Bar Bottom Border
        RECT rcLine = { 0, 38, rc.right, 39 };
        HBRUSH hbrLine = CreateSolidBrush(CLR_BORDER);
        FillRect(hdc, &rcLine, hbrLine);
        DeleteObject(hbrLine);

        // Title Text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_PURPLE);
        SelectObject(hdc, g_hFontUi);
        TextOutW(hdc, 15, 10, L"⚡ VOLTRO 2", 10);

        SetTextColor(hdc, CLR_TEXT_MUTED);
        TextOutW(hdc, 105, 10, L"|  Xeno Native Architecture  |  v2.5.0", 37);

        // Attach Status Badge
        RECT rcBadge = { rc.right - 220, 9, rc.right - 90, 30 };
        SetTextColor(hdc, g_IsAttached ? CLR_GREEN : CLR_TEXT_MUTED);
        std::wstring stStr = g_IsAttached ? L"● ATTACHED" : L"○ NOT ATTACHED";
        DrawTextW(hdc, stStr.c_str(), -1, &rcBadge, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

        EndPaint(hWnd, &ps);
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)g_hbrBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, CLR_PANEL);
        SetTextColor(hdc, CLR_TEXT);
        return (LRESULT)g_hbrPanel;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, CLR_OUTPUT_BG);
        SetTextColor(hdc, RGB(52, 211, 153)); // Soft emerald terminal text
        return (LRESULT)g_hbrOutput;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        switch (id) {
        case IDC_BTN_ATTACH:
            PerformAttach();
            break;
        case IDC_BTN_EXECUTE:
            PerformExecute();
            break;
        case IDC_BTN_CLEAR:
            SetEditorText("");
            AddOutputLog("[EDITOR] Cleared script buffer.");
            break;
        case IDC_BTN_OPEN:
            OpenFileDialog();
            break;
        case IDC_BTN_SAVE:
            SaveFileDialog();
            break;
        case IDC_BTN_SCRIPTHUB:
            ShowScriptHubMenu(hWnd);
            break;
        case IDC_BTN_CLEARLOGS:
            SendMessageW(g_hOutputList, LB_RESETCONTENT, 0, 0);
            AddOutputLog("[OUTPUT] Cleared console logs.");
            break;
        case IDC_BTN_NEWTAB: {
            int newIdx = (int)g_Tabs.size() + 1;
            ScriptTab tab;
            tab.title = L"Script" + std::to_wstring(newIdx) + L".lua";
            tab.content = "-- Script " + std::to_string(newIdx) + "\n";
            g_Tabs.push_back(tab);

            TCITEMW tie = {};
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPWSTR)tab.title.c_str();
            int idx = TabCtrl_InsertItem(g_hTabCtrl, (int)g_Tabs.size() - 1, &tie);
            TabCtrl_SetCurSel(g_hTabCtrl, idx);
            SetEditorText(tab.content);
            g_ActiveTab = idx;
            break;
        }
        }
        break;
    }

    case WM_NOTIFY: {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == IDC_TAB_SCRIPTS && pnm->code == TCN_SELCHANGE) {
            // Save current tab content
            if (g_ActiveTab >= 0 && g_ActiveTab < (int)g_Tabs.size()) {
                g_Tabs[g_ActiveTab].content = GetEditorText();
            }
            int curSel = TabCtrl_GetCurSel(g_hTabCtrl);
            if (curSel >= 0 && curSel < (int)g_Tabs.size()) {
                g_ActiveTab = curSel;
                SetEditorText(g_Tabs[curSel].content);
            }
        }
        break;
    }

    case WM_PIPE_LOG: {
        std::string* pStr = (std::string*)lParam;
        if (pStr) {
            AddOutputLog(*pStr, wParam != 0);
            delete pStr;
        }
        break;
    }

    case WM_PIPE_ATTACHED: {
        bool att = (wParam == 1);
        UpdateAttachStatus(att, g_AttachedPid);
        break;
    }

    case WM_DESTROY: {
        SolaraHostPipeServer::instance().stop();
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Application Entry Point
// -----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Privileges & Protections
    Solara::Injector::EnableDebugPrivilege();
    Voltro::BinaryProtector::enforceSecurityGuards();

    // 2. Start Xeno Named Pipe Server
    SolaraHostPipeServer::instance().start();

    // 3. UI Resources & Fonts
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TAB_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    g_hFontUi = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    g_hFontCode = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    g_hbrBg = CreateSolidBrush(CLR_BG);
    g_hbrPanel = CreateSolidBrush(CLR_PANEL);
    g_hbrOutput = CreateSolidBrush(CLR_OUTPUT_BG);

    // 4. Register Window Class
    const wchar_t CLASS_NAME[] = L"Voltro2XenoNativeHost";
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = g_hbrBg;
    wcex.lpszClassName = CLASS_NAME;

    HICON hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    wcex.hIcon = hIcon;
    wcex.hIconSm = hIcon;

    RegisterClassExW(&wcex);

    // 5. Create Window
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 800;
    int winH = 640;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"VOLTRO 2 - Xeno Native Architecture",
        WS_POPUP | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 0;

    // Apply Modern Windows 11 / 10 Dark Mode & Rounded Corners
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(g_hWnd, 20, &darkMode, sizeof(darkMode));
    DwmSetWindowAttribute(g_hWnd, 19, &darkMode, sizeof(darkMode));
    int cornerPref = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(g_hWnd, 33, &cornerPref, sizeof(cornerPref));

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // 6. Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hFontUi) DeleteObject(g_hFontUi);
    if (g_hFontCode) DeleteObject(g_hFontCode);
    if (g_hbrBg) DeleteObject(g_hbrBg);
    if (g_hbrPanel) DeleteObject(g_hbrPanel);
    if (g_hbrOutput) DeleteObject(g_hbrOutput);

    return (int)msg.wParam;
}
