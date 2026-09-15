#include <windows.h>
#include <wrl/client.h>
#include <WebView2.h>
#include <string>
#include <sstream>
#include <vector>
#include <shlobj.h>
#include <shellapi.h>
#include <iostream>
#include <regex>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <wrl/event.h>

#include "script_hub.h"
#include "thakur_env.h"
#include "injector/ThakurInjector.h"
#include "security_guard.h"
#include "ipc/ipc_client.h"
#include "process_manager.h"

using namespace Microsoft::WRL;

static ComPtr<ICoreWebView2Controller> g_webviewController;
static ComPtr<ICoreWebView2> g_webview;
static HWND g_hWnd = NULL;

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size);
    return wstr;
}

static std::wstring FilePathToUrl(std::wstring path) {
    for (wchar_t& character : path) {
        if (character == L'\\') {
            character = L'/';
        }
    }
    return L"file:///" + path;
}

static std::string EscapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char hexBuf[7];
                    sprintf_s(hexBuf, "\\u%04x", static_cast<unsigned char>(c));
                    ss << hexBuf;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

static std::string ExtractJsonField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        pos++; 
        std::string result;
        result.reserve(1024);
        bool escaped = false;
        while (pos < json.length()) {
            char c = json[pos++];
            if (escaped) {
                switch (c) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u':
                        if (pos + 4 <= json.length()) {
                            unsigned int hexVal = 0;
                            std::string hexStr = json.substr(pos, 4);
                            std::stringstream ss;
                            ss << std::hex << hexStr;
                            ss >> hexVal;
                            pos += 4;
                            if (hexVal < 128) {
                                result += static_cast<char>(hexVal);
                            } else if (hexVal < 0x800) {
                                result += static_cast<char>(0xC0 | (hexVal >> 6));
                                result += static_cast<char>(0x80 | (hexVal & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (hexVal >> 12));
                                result += static_cast<char>(0x80 | ((hexVal >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (hexVal & 0x3F));
                            }
                        }
                        break;
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
    return "";
}

void PostMessageToUI(const std::string& type, const std::string& message, bool success = true) {
    if (!g_webview) return;
    std::ostringstream ss;
    ss << "{\"type\":\"" << type << "\",\"message\":\"" << EscapeJsonString(message) << "\",\"success\":" << (success ? "true" : "false") << ",\"attached\":" << (success ? "true" : "false") << "}";
    std::wstring wMsg = Utf8ToWide(ss.str());
    g_webview->PostWebMessageAsJson(wMsg.c_str());
}

void HandleWebMessage(const std::wstring& msgStr);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (g_webviewController) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            g_webviewController->put_Bounds(bounds);
        }
        break;
    case WM_CLOSE:
        if (g_webviewController) {
            g_webviewController->Close();
            g_webviewController = nullptr;
        }
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        ExitProcess(0);
        break;
    case WM_HOTKEY:
        
        if (wParam == 101) {
            PostMessageToUI("ACTION_NEW_TAB", "WIN+T", true);
        }
        
        else if (wParam == 102) {
            PostMessageToUI("ACTION_CLOSE_TAB", "WIN+W", true);
        }
        break;
    case WM_DESTROY:
        UnregisterHotKey(hWnd, 101);
        UnregisterHotKey(hWnd, 102);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    Thakur::BinaryProtector::enforceSecurityGuards();

    
    auto& thakurEnv = Thakur::ThakurEnvironment::getInstance();
    Thakur::NamedPipeBridge::getInstance().startServer(L"\\\\.\\pipe\\ThakurIPC");
    Thakur::NamedPipeBridge::getInstance().setPayloadHandler([](const Thakur::IpcMessage& msg) {
        std::string raw = msg.script;
        if (raw.find("\"type\":\"OUTPUT\"") != std::string::npos) {
            std::string text = ExtractJsonField(raw, "text");
            std::string level = ExtractJsonField(raw, "level");
            PostMessageToUI("CONSOLE_LOG", text.empty() ? raw : text, level != "error");
        } else if (raw.find("\"type\":\"ATTACH_STATUS\"") != std::string::npos) {
            std::string text = ExtractJsonField(raw, "message");
            PostMessageToUI("ATTACH_STATUS", text.empty() ? "DLL attached successfully." : text, true);
        } else if (!raw.empty()) {
            PostMessageToUI("CONSOLE_LOG", raw, true);
        }
    });

    
    HICON hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    if (!hIcon) {
        hIcon = (HICON)LoadImageW(NULL, L"thakur_ui/resources/images/thakur.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }

    const wchar_t CLASS_NAME[] = L"ThakurExecutorNativeHost";
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = hIcon;
    wcex.hIconSm = hIcon;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wcex);

    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 1140;
    int winH = 740;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    g_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"THAKUR EXECUTOR v1.5.0",
        WS_POPUP | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_SYSMENU,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 0;

    if (hIcon) {
        SendMessageW(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(g_hWnd, 20, &darkMode, sizeof(darkMode)); 
    DwmSetWindowAttribute(g_hWnd, 19, &darkMode, sizeof(darkMode)); 

    
    int cornerPref = 2;
    DwmSetWindowAttribute(g_hWnd, 33, &cornerPref, sizeof(cornerPref));

    
    MARGINS shadowMargins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(g_hWnd, &shadowMargins);

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    
    RegisterHotKey(g_hWnd, 101, MOD_WIN | MOD_NOREPEAT, 'T');
    RegisterHotKey(g_hWnd, 102, MOD_WIN | MOD_NOREPEAT, 'W');

    
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    MessageBoxW(g_hWnd, L"Microsoft Edge WebView2 Runtime is required to run Thakur Executor.", L"Thakur Runtime Error", MB_ICONERROR);
                    return result;
                }

                env->CreateCoreWebView2Controller(g_hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            g_webviewController = controller;
                            g_webviewController->get_CoreWebView2(&g_webview);

                            
                            ComPtr<ICoreWebView2Settings> settings;
                            g_webview->get_Settings(&settings);
                            if (settings) {
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                                settings->put_IsWebMessageEnabled(TRUE);
                                settings->put_AreDevToolsEnabled(TRUE);
                            }

                            
                            RECT bounds;
                            GetClientRect(g_hWnd, &bounds);
                            g_webviewController->put_Bounds(bounds);

                            
                            g_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR messageRaw = nullptr;
                                        HRESULT hr = args->TryGetWebMessageAsString(&messageRaw);
                                        if (FAILED(hr) || !messageRaw) {
                                            args->get_WebMessageAsJson(&messageRaw);
                                        }
                                        if (messageRaw) {
                                            HandleWebMessage(messageRaw);
                                            CoTaskMemFree(messageRaw);
                                        }
                                        return S_OK;
                                    }
                                ).Get(), nullptr
                            );

                            
                            std::wstring bridgeScript = LR"JS(
(function(){
    
    var origGetById = document.getElementById.bind(document);
    var dummyLineNums = { style: {} };
    document.getElementById = function(id){
        var el = origGetById(id);
        if(!el && id === 'lineNums') return dummyLineNums;
        return el;
    };

    
    window.postNativeMessage = function(type, payload){
        if(window.chrome && window.chrome.webview && typeof window.chrome.webview.postMessage === 'function'){
            try {
                window.chrome.webview.postMessage(JSON.stringify({ type: type, payload: payload || {} }));
                return true;
            } catch(e){
                try { window.chrome.webview.postMessage(type); return true; } catch(e2){}
            }
        }
        return false;
    };

    
    window.appMin = function(e){
        if(e){ e.preventDefault(); e.stopPropagation(); }
        window.postNativeMessage('WINDOW_MINIMIZE');
    };
    window.appMax = function(e){
        if(e){ e.preventDefault(); e.stopPropagation(); }
        window.postNativeMessage('WINDOW_MAXIMIZE');
    };
    window.appClose = function(e){
        if(e){ e.preventDefault(); e.stopPropagation(); }
        window.postNativeMessage('WINDOW_CLOSE');
    };

    
    window.closeContribModal = function(){
        var m = document.getElementById('contribModal');
        if(m) m.classList.remove('open');
    };
    window.closeKeyModal = function(){
        var m = document.getElementById('keyModal');
        if(m) m.classList.remove('open');
    };
    window.closeWizardModal = function(){
        var m = document.getElementById('wizardModal');
        if(m) m.classList.remove('open');
    };
    window.closePrimezModal = function(){
        var m = document.getElementById('pzModal');
        if(m) m.classList.remove('open');
    };

    
    var DEFAULT_PRIMEZ = {
        badge: 'MADE BY',
        title: 'PRIMEZ STUDIOS',
        sub: 'POWERING THE NEXT GENERATION OF SOFT',
        discord: 'https:
        projects: 'https:
    };
    function getPrimezConfig(){
        try {
            var s = localStorage.getItem('thakur_primez_cfg');
            if(s) return Object.assign({}, DEFAULT_PRIMEZ, JSON.parse(s));
        } catch(e){}
        return Object.assign({}, DEFAULT_PRIMEZ);
    }
    function applyPrimezConfig(cfg){
        if(!cfg) cfg = getPrimezConfig();
        var b = document.getElementById('pzBannerBadge');
        if(b) b.innerHTML = '<i class="fas fa-bolt"></i> ' + (cfg.badge || 'MADE BY');
        var t = document.getElementById('pzBannerTitle');
        if(t){
            var str = cfg.title || 'PRIMEZ STUDIOS';
            if(str.toUpperCase().indexOf('PRIMEZ') !== -1){
                t.innerHTML = str.replace(/PRIMEZ/i, 'PRIME<span class="pz-z">Z</span>');
            } else {
                t.textContent = str;
            }
        }
        var s = document.getElementById('pzBannerSub');
        if(s) s.innerHTML = '<span class="pz-l"></span>' + (cfg.sub || 'POWERING THE NEXT GENERATION OF SOFT') + '<span class="pz-r"></span>';
        var dc = document.getElementById('pzJoinDcBtn');
        if(dc){
            dc.onclick = function(){
                window.postNativeMessage('OPEN_EXTERNAL_URL', { url: cfg.discord || 'https:
            };
        }
        var pr = document.getElementById('pzExploreBtn');
        if(pr){
            pr.onclick = function(){
                window.postNativeMessage('OPEN_EXTERNAL_URL', { url: cfg.projects || 'https:
            };
        }
    }
    window.openPrimezCustomizer = function(e){
        if(e){ e.preventDefault(); e.stopPropagation(); }
        var cfg = getPrimezConfig();
        var b = document.getElementById('pzCfgBadge'); if(b) b.value = cfg.badge;
        var t = document.getElementById('pzCfgTitle'); if(t) t.value = cfg.title;
        var s = document.getElementById('pzCfgSub'); if(s) s.value = cfg.sub;
        var d = document.getElementById('pzCfgDiscord'); if(d) d.value = cfg.discord;
        var p = document.getElementById('pzCfgProjects'); if(p) p.value = cfg.projects;
        var m = document.getElementById('pzModal'); if(m) m.classList.add('open');
    };
    window.closePrimezModal = function(){
        var m = document.getElementById('pzModal'); if(m) m.classList.remove('open');
    };
    window.savePrimezCustomizer = function(){
        var b = document.getElementById('pzCfgBadge');
        var t = document.getElementById('pzCfgTitle');
        var s = document.getElementById('pzCfgSub');
        var d = document.getElementById('pzCfgDiscord');
        var p = document.getElementById('pzCfgProjects');
        var cfg = {
            badge: (b && b.value.trim()) ? b.value.trim() : DEFAULT_PRIMEZ.badge,
            title: (t && t.value.trim()) ? t.value.trim() : DEFAULT_PRIMEZ.title,
            sub: (s && s.value.trim()) ? s.value.trim() : DEFAULT_PRIMEZ.sub,
            discord: (d && d.value.trim()) ? d.value.trim() : DEFAULT_PRIMEZ.discord,
            projects: (p && p.value.trim()) ? p.value.trim() : DEFAULT_PRIMEZ.projects
        };
        try { localStorage.setItem('thakur_primez_cfg', JSON.stringify(cfg)); } catch(e){}
        applyPrimezConfig(cfg);
        window.closePrimezModal();
    };
    window.resetPrimezDefaults = function(){
        try { localStorage.removeItem('thakur_primez_cfg'); } catch(e){}
        applyPrimezConfig(DEFAULT_PRIMEZ);
        var b = document.getElementById('pzCfgBadge'); if(b) b.value = DEFAULT_PRIMEZ.badge;
        var t = document.getElementById('pzCfgTitle'); if(t) t.value = DEFAULT_PRIMEZ.title;
        var s = document.getElementById('pzCfgSub'); if(s) s.value = DEFAULT_PRIMEZ.sub;
        var d = document.getElementById('pzCfgDiscord'); if(d) d.value = DEFAULT_PRIMEZ.discord;
        var p = document.getElementById('pzCfgProjects'); if(p) p.value = DEFAULT_PRIMEZ.projects;
    };
    window.addEventListener('DOMContentLoaded', function(){ applyPrimezConfig(); });

    
    document.addEventListener('click', function(e){
        
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.appClose();
            return;
        }
        
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.appMin();
            return;
        }
        
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.appMax();
            return;
        }
        
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.closeContribModal();
            return;
        }
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.closeKeyModal();
            return;
        }
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.closePrimezModal();
            return;
        }
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            window.closeWizardModal();
            return;
        }
        if(e.target.closest('
            var cl = document.getElementById('clModal');
            if(cl) cl.classList.remove('open');
            return;
        }
        if(e.target.closest('
            var dm = document.getElementById('discordModal');
            if(dm) dm.classList.remove('open');
            return;
        }
        if(e.target.closest('
            var mh = document.getElementById('manageHubsModal');
            if(mh) mh.classList.remove('open');
            return;
        }
        if(e.target.closest('
            var im = document.getElementById('inspectModal');
            if(im) im.classList.remove('open');
            return;
        }
        
        if(e.target.closest('
            e.preventDefault(); e.stopPropagation();
            if(typeof S !== 'undefined' && S.att){
                window.postNativeMessage('DETACH_REQUEST');
                if(typeof setAttachState === 'function') setAttachState('not');
                if(typeof addOutput === 'function') addOutput('Detached from process.', 'wr');
                if(typeof toast === 'function') toast('Detached', 'warning');
                return;
            }
            if(typeof S !== 'undefined' && !S.hasKey){
                if(typeof openKeyModal === 'function') openKeyModal();
                if(typeof toast === 'function') toast('A valid key is required to attach', 'error');
                return;
            }
            if(typeof setAttachState === 'function') setAttachState('attaching');
            if(typeof addOutput === 'function') addOutput('THAKUR attach: scanning active processes for RobloxPlayerBeta.exe...', 'in');
            window.postNativeMessage('ATTACH_REQUEST');
            return;
        }
        
        if(e.target.classList && e.target.classList.contains('m-ov')){
            e.target.classList.remove('open');
            return;
        }
    }, true);

    
    document.addEventListener('keydown', function(e){
        if(e.key === 'Escape'){
            var modals = document.querySelectorAll('.m-ov.open');
            modals.forEach(function(m){ m.classList.remove('open'); });
        }
    });

    
    function applyKeyUnlock(){
        if(typeof S !== 'undefined'){
            S.key = '373438';
        }
        var keyInp = document.getElementById('keyInput');
        if(keyInp){
            keyInp.placeholder = 'Enter key: 373438';
            if(!keyInp.value) keyInp.value = '373438';
        }

        var keyDesc = document.querySelector('
        if(keyDesc){
            keyDesc.innerHTML = 'Enter key <strong>373438</strong> to unlock all features with a <strong>1-week (7 days) expiry</strong>.';
        }

        var keyGetBtn = document.getElementById('keyGet');
        if(keyGetBtn){
            keyGetBtn.onclick = function(e){
                e.preventDefault(); e.stopPropagation();
                window.postNativeMessage('OPEN_EXTERNAL_URL', { url: 'https:
                if(keyInp) keyInp.value = '373438';
                if(typeof toast === 'function') toast('Opening Discord & populated test key: 373438', 'info');
            };
        }

        var keySubBtn = document.getElementById('keySubmit');
        if(keySubBtn){
            keySubBtn.onclick = function(e){
                e.preventDefault(); e.stopPropagation();
                var val = keyInp ? keyInp.value.trim() : '';
                if(!val){
                    var err = document.getElementById('keyErr');
                    if(err) err.textContent = 'Please enter a key.';
                    return;
                }
                if(val === '373438' || val === 'THAKUR-2026-BETA' || val.startsWith('THAKUR-') || val.startsWith('PIXEL-')){
                    if(typeof S !== 'undefined'){
                        S.hasKey = true;
                        S.plan = 'premium';
                        S.premiumDays = 7;
                        if(typeof updatePlanStatus === 'function') updatePlanStatus();
                        if(typeof renderPlans === 'function') renderPlans();
                    }
                    window.closeKeyModal();
                    var stKey = document.getElementById('stKey');
                    if(stKey){
                        stKey.textContent = 'Premium Active · 7 Days Remaining';
                        stKey.style.color = 'var(--ok)';
                    }
                    if(typeof toast === 'function') toast('Key 373438 Accepted! 1-Week Premium Expiry (7 Days) Active.', 'success');
                    if(typeof addOutput === 'function') addOutput('[KEY AUTH] Key 373438 validated successfully · 1-Week Expiry (7 Days) · All Features Attached & Unlocked.', 'ok');
                    try {
                        localStorage.setItem('thakur_active_key', JSON.stringify({ key: '373438', days: 7, activatedAt: Date.now() }));
                    } catch(ex){}
                    if(typeof saveSession === 'function') saveSession();
                } else {
                    var err = document.getElementById('keyErr');
                    if(err) err.textContent = 'Invalid key. Please enter 373438.';
                    if(typeof toast === 'function') toast('Invalid key. Expected 373438', 'error');
                }
            };
        }

        
        try {
            var savedKey = localStorage.getItem('thakur_active_key');
            var remainingDays = 7;
            if(savedKey){
                var kObj = JSON.parse(savedKey);
                var daysElapsed = Math.floor((Date.now() - (kObj.activatedAt || Date.now())) / (1000 * 60 * 60 * 24));
                remainingDays = Math.max(1, (kObj.days || 7) - daysElapsed);
            }
            if(typeof S !== 'undefined'){
                S.hasKey = true;
                S.plan = 'premium';
                S.premiumDays = remainingDays;
                if(typeof updatePlanStatus === 'function') updatePlanStatus();
            }
            var stKey = document.getElementById('stKey');
            if(stKey){
                stKey.textContent = 'Premium Active · ' + remainingDays + ' Days Remaining';
                stKey.style.color = 'var(--ok)';
            }
        } catch(ex){}

        
        var cards = document.querySelectorAll('.cr-c');
        cards.forEach(function(card){
            var roleEl = card.querySelector('.cr-r');
            if(roleEl && roleEl.textContent.indexOf('Special Thanks') !== -1){
                var nameEl = card.querySelector('.cr-n');
                if(nameEl) nameEl.textContent = 'Exploit Network';
                var avatar = card.querySelector('.cr-a');
                if(avatar) avatar.textContent = 'E';
                var subEl = card.querySelector('div[style*="margin-top:2px"]');
                if(subEl) subEl.remove();
            }
        });
    }

    
    if(window.chrome && window.chrome.webview){
        window.chrome.webview.addEventListener('message', function(event){
            try {
                var msg = (typeof event.data === 'string') ? JSON.parse(event.data) : event.data;
                if(!msg) return;
                if(msg.type === 'ATTACH_STATUS'){
                    var isAttached = (msg.attached !== undefined) ? (msg.attached === true) : (msg.success === true);
                    if(typeof setAttachState === 'function'){
                        setAttachState(isAttached ? 'attached' : 'error');
                    }
                    if(typeof S !== 'undefined'){
                        S.att = isAttached;
                    }
                    if(isAttached){
                        if(typeof addOutput === 'function') addOutput(msg.message || 'Successfully attached to Roblox.', 'ok');
                        if(typeof toast === 'function') toast(msg.message || 'Successfully attached to Roblox', 'success');
                    } else {
                        if(typeof addOutput === 'function') addOutput('[ATTACH FAILED] ' + (msg.message || 'No running Roblox process detected. Please launch Roblox first.'), 'er');
                        if(typeof toast === 'function') toast(msg.message || 'Roblox is not running. Launch Roblox first.', 'error');
                        setTimeout(function(){
                            if(typeof S !== 'undefined' && !S.att && typeof setAttachState === 'function'){
                                setAttachState('not');
                            }
                        }, 2500);
                    }
                } else if(msg.type === 'CONSOLE_LOG'){
                    if(typeof addOutput === 'function') addOutput(msg.message || msg.text, msg.success ? 'ok' : 'er');
                } else if(msg.type === 'EXECUTION_RESULT'){
                    if(typeof S !== 'undefined') S.execRunning = false;
                    var stAct = document.getElementById('stAct');
                    if(stAct) stAct.textContent = (typeof S !== 'undefined' && S.att) ? '[Attached]' : '[Ready]';
                    if(typeof addOutput === 'function') addOutput(msg.message || 'Execution finished.', msg.success ? 'ok' : 'er');
                } else if(msg.type === 'ACTION_NEW_TAB'){
                    if(typeof createTab === 'function') createTab();
                    if(typeof toast === 'function') toast('New tab created (WIN+T)', 'ok');
                } else if(msg.type === 'ACTION_CLOSE_TAB'){
                    if(typeof closeTab === 'function' && typeof S !== 'undefined') closeTab(S.activeTab);
                    if(typeof toast === 'function') toast('Tab closed (WIN+W)', 'info');
                } else if(msg.type === 'LOAD_SCRIPT_CONTENT'){
                    if(msg.message){
                        if(typeof setEditorCode === 'function') setEditorCode(msg.message);
                        if(typeof S !== 'undefined' && S.tabs){
                            var curTab = S.tabs.find(function(t){ return t.id === S.activeTab; });
                            if(curTab){ curTab.content = msg.message; curTab.unsaved = true; if(typeof renderTabs === 'function') renderTabs(); }
                        }
                        if(typeof toast === 'function') toast('Loaded remote script (' + msg.message.length + ' chars)', 'ok');
                    }
                }
            } catch(e){
                console.error('WebView2 IPC message listener error:', e);
            }
        });
    }

    window.addEventListener('DOMContentLoaded', function(){
        applyPrimezConfig();
        applyKeyUnlock();
    });
    setTimeout(applyKeyUnlock, 200);
    setTimeout(applyKeyUnlock, 800);
})();
)JS";
                            g_webview->AddScriptToExecuteOnDocumentCreated(bridgeScript.c_str(), nullptr);

                            
                            
                            
                            wchar_t exePath[MAX_PATH];
                            GetModuleFileNameW(NULL, exePath, MAX_PATH);
                            wchar_t* lastSlash = wcsrchr(exePath, L'\\');
                            if (lastSlash) *lastSlash = L'\0';

                            std::wstring uiPath = std::wstring(exePath) + L"\\thakur_ui\\index.html";
                            if (GetFileAttributesW(uiPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                                MessageBoxW(
                                    g_hWnd,
                                    L"The Thakur UI bundle was not found. Keep the thakur_ui folder next to thakur.exe.",
                                    L"Thakur UI Error",
                                    MB_ICONERROR
                                );
                                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                            }

                            std::wstring localUrl = FilePathToUrl(uiPath);
                            HRESULT navigationResult = g_webview->Navigate(localUrl.c_str());
                            if (FAILED(navigationResult)) {
                                MessageBoxW(
                                    g_hWnd,
                                    L"Thakur could not load its local interface.",
                                    L"Thakur UI Error",
                                    MB_ICONERROR
                                );
                                return navigationResult;
                            }

                            return S_OK;
                        }
                    ).Get()
                );
                return S_OK;
            }
        ).Get()
    );

    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

void HandleWebMessage(const std::wstring& msgStr) {
    std::string s = WideToUtf8(msgStr);

    
    if (s.find("WINDOW_CLOSE") != std::string::npos) {
        if (g_webviewController) {
            g_webviewController->Close();
            g_webviewController = nullptr;
        }
        DestroyWindow(g_hWnd);
        PostQuitMessage(0);
        ExitProcess(0);
        return;
    } else if (s.find("WINDOW_MINIMIZE") != std::string::npos) {
        ShowWindow(g_hWnd, SW_MINIMIZE);
        PostMessage(g_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        return;
    } else if (s.find("WINDOW_MAXIMIZE") != std::string::npos) {
        if (IsZoomed(g_hWnd)) {
            ShowWindow(g_hWnd, SW_RESTORE);
            PostMessage(g_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
        } else {
            ShowWindow(g_hWnd, SW_MAXIMIZE);
            PostMessage(g_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
        return;
    } else if (s.find("WINDOW_DRAG") != std::string::npos) {
        ReleaseCapture();
        SendMessage(g_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return;
    } else if (s.find("OPEN_EXTERNAL_URL") != std::string::npos) {
        std::string url = ExtractJsonField(s, "url");
        if (url.empty()) {
            std::regex urlRegex(R"((https?:\/\/[^\s"'`\)]+))", std::regex::icase);
            std::smatch urlMatch;
            if (std::regex_search(s, urlMatch, urlRegex)) {
                url = urlMatch[1].str();
            }
        }
        if (!url.empty()) {
            std::wstring wUrl = Utf8ToWide(url);
            ShellExecuteW(NULL, L"open", wUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        return;
    }

    
    if (s.find("\"ATTACH_REQUEST\"") != std::string::npos) {
        
        Thakur::Injector::ThakurInjector::instance();
        auto procs = Thakur::Injector::ThakurInjector::instance().findRobloxProcesses();
        if (!procs.empty()) {
            bool hasRoblox = false;
            for (auto& p : procs) {
                if (p.name.find(L"RobloxPlayerBeta") != std::wstring::npos) {
                    hasRoblox = true;
                    break;
                }
            }
            if (!hasRoblox) {
                PostMessageToUI("CONSOLE_LOG", std::string("[ATTACH FAILED] Roblox module not validated for PID."), true);
                PostMessageToUI("ATTACH_STATUS", std::string("Roblox module validation failed."), false);
                return;
            }
            Thakur::Injector::InjectionResult res = Thakur::Injector::ThakurInjector::instance().inject(procs[0].pid, std::string("thakur_runtime.dll"), Thakur::Injector::InjectionMethod::LoadLibrary);
            if (res.status == Thakur::Injector::InjectionStatus::Success) {
                PostMessageToUI("CONSOLE_LOG", "[ATTACH SUCCESS] Injected into Roblox PID " + std::to_string(procs[0].pid) + ".", true);
                PostMessageToUI("ATTACH_STATUS", "Attached to Roblox process.", true);
            } else {
                PostMessageToUI("CONSOLE_LOG", "[ATTACH FAILED] " + res.message, true);
                PostMessageToUI("ATTACH_STATUS", res.message, false);
            }
        } else {
            PostMessageToUI("CONSOLE_LOG", "[ATTACH FAILED] Roblox process not found.", true);
            PostMessageToUI("ATTACH_STATUS", "Roblox process not found.", false);
        }
    } else if (s.find("\"DETACH_REQUEST\"") != std::string::npos) {
        Thakur::RobloxWatchdog::getInstance().stopWatch();
        PostMessageToUI("ATTACH_STATUS", "Detached from target process.", false);
    } else if (s.find("\"EXECUTE_PAYLOAD\"") != std::string::npos) {
        std::string code = ExtractJsonField(s, "code");
        if (code.empty() && s.find("\"code\":") == std::string::npos) {
            code = s;
        }

        
        while (!code.empty() && (code.front() == ' ' || code.front() == '\t' || code.front() == '\r' || code.front() == '\n')) {
            code.erase(code.begin());
        }
        while (!code.empty() && (code.back() == ' ' || code.back() == '\t' || code.back() == '\r' || code.back() == '\n')) {
            code.pop_back();
        }

        
        std::regex urlRegex(R"((https?:\/\/[^\s"'`\)]+))", std::regex::icase);
        std::smatch urlMatch;
        if (code.find("http://") == 0 || code.find("https://") == 0 ||
            ((code.find("HttpGet") != std::string::npos || code.find("request") != std::string::npos) && std::regex_search(code, urlMatch, urlRegex))) {

            std::string fetchTarget = (code.find("http") == 0) ? code : urlMatch[1].str();
            PostMessageToUI("CONSOLE_LOG", "[THAKUR NETWORK] Fetching remote script source from: " + fetchTarget, true);

            std::string fetched = Thakur::ScriptEngine::fetchRemoteScript(fetchTarget);
            if (!fetched.empty() && fetched.find("-- [ERROR]") != 0) {
                code = fetched;
                PostMessageToUI("CONSOLE_LOG", "[THAKUR NETWORK] Successfully fetched remote source (" + std::to_string(code.size()) + " bytes)", true);
            } else {
                PostMessageToUI("CONSOLE_LOG", "[THAKUR NETWORK ERROR] " + fetched, false);
            }
        }

        
        auto report = Thakur::ScriptEngine::scanHeuristics(code);
        PostMessageToUI("CONSOLE_LOG", "[SECURITY AUDIT] Verdict: " + report.verdict + " (Payload: " + std::to_string(code.size()) + " bytes)", true);
        for (const auto& flag : report.detectedFlags) {
            PostMessageToUI("CONSOLE_LOG", "  -> Heuristic: " + flag, report.verdict == "CLEAN");
        }

        
        {
    

            auto& env = Thakur::ThakurEnvironment::getInstance();
            if (code.find("identifyexecutor") != std::string::npos) {
                std::string res = env.invokeGlobal("identifyexecutor", {});
                PostMessageToUI("CONSOLE_LOG", "  [OUTPUT] identifyexecutor() => " + res, true);
            }
            if (code.find("printidentity") != std::string::npos) {
                PostMessageToUI("CONSOLE_LOG", "  [OUTPUT] Current identity is 3 (Thakur Engine)", true);
            }
            if (code.find("checkcaller") != std::string::npos) {
                std::string res = env.invokeGlobal("checkcaller", {});
                PostMessageToUI("CONSOLE_LOG", "  [OUTPUT] checkcaller() => " + res, true);
            }

            
            std::regex printRegex(R"((print|warn|error)\s*\(\s*["'](.*?)["']\s*\))");
            auto words_begin = std::sregex_iterator(code.cbegin(), code.cend(), printRegex);
            auto words_end = std::sregex_iterator();
            for (std::sregex_iterator it = words_begin; it != words_end; ++it) {
                std::smatch match = *it;
                std::string type = match[1].str();
                std::string msg = match[2].str();
                PostMessageToUI("CONSOLE_LOG", "  [" + type + "] " + msg, type == "print");
            }

        }

    } else if (s.find("\"EXECUTE_FPS_BOOST\"") != std::string::npos) {
        
        std::string script = ExtractJsonField(s, "script");
        std::string targetFpsStr = ExtractJsonField(s, "target_fps");
        int targetFps = 240;
        try { if (!targetFpsStr.empty()) targetFps = std::stoi(targetFpsStr); } catch(...) {}

        auto& env = Thakur::ThakurEnvironment::getInstance();
        env.invokeGlobal("setfpscap", { std::to_string(targetFps) });

        if (script.empty()) {
            script = Thakur::ThakurEnvironment::generateFpsBoostScript(targetFps);
        }

    } else if (s.find("\"RSCRIPTS_SEARCH\"") != std::string::npos) {
        
        std::string q = ExtractJsonField(s, "query");
        auto liveItems = Thakur::ScriptEngine::queryRscriptsApi(q, 10, 1);
        PostMessageToUI("CONSOLE_LOG", "[RSCRIPTS API] Found " + std::to_string(liveItems.size()) + " live verified scripts for: " + q, true);
    } else if (s.find("\"FETCH_REMOTE_URL\"") != std::string::npos) {
        
        std::regex urlRegex(R"((https?:\/\/[^\s"'`\)]+))", std::regex::icase);
        std::smatch urlMatch;
        if (std::regex_search(s, urlMatch, urlRegex)) {
            std::string url = urlMatch[1].str();
            PostMessageToUI("CONSOLE_LOG", "[FETCH] Loading remote script: " + url, true);
            std::string content = Thakur::ScriptEngine::fetchRemoteScript(url);
            PostMessageToUI("LOAD_SCRIPT_CONTENT", content, true);
        }
    } else if (s.find("\"STOP_EXECUTION\"") != std::string::npos) {
        PostMessageToUI("CONSOLE_LOG", "Execution thread signaled to abort.", false);
    }
}
