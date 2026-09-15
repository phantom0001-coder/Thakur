

High-performance, native Windows-based Roblox script executor designed to provide a modern, secure, and intuitive experience for executing Lua scripts in Roblox games.

---

- **Native Qt6 C++ Interface**: Frameless, modern dark theme with custom neon-green (`
- **Advanced Code Editor**: Syntax highlighting for Luau/Roblox APIs, line numbers, multi-tab workspace, auto-indentation, and find/replace.
- **Dynamic Multi-Hub Aggregator**: Search verified scripts from `rscripts.net`, `scripthub.com`, `robloxscripts.com`, and `scriptblox.com` with real-time exploit security heuristics analysis.
- **Stealth Injection & Execution Engine**: Manual mapping & process hollowing evasion, dynamic pattern scanning for `lua_State*` and `luaL_dostring`, with Named Pipes (`\\.\pipe\ThakurIPC`) for robust IPC.
- **Hardware ID (HWID) & Premium Key System**: Tiered access with offline caching, local encryption, and server-side verification.
- **Background Auto-Updater**: Automated release checks with SHA-256 checksum verification and automatic rollbacks.

---

```
thakur/
├── CMakeLists.txt                 
├── build_thakur.ps1               
├── .github/workflows/             
├── thakur_ui/
│   ├── resources.qrc              
│   ├── resources/
│   │   ├── images/                
│   │   └── themes/                
│   ├── ui/                        
│   └── src/
│       ├── main_qt.cpp            
│       ├── mainwindow.h/.cpp      
│       ├── ThemeManager.h/.cpp    
│       ├── EditorWidget.h/.cpp    
│       ├── splashscreen.h/.cpp    
│       ├── hub/
│       │   ├── HubManager.h/.cpp  
│       │   └── ScriptCard.h/.cpp  
│       ├── injector/
│       │   ├── ThakurInjector.h/.cpp 
│       │   └── patterns.json      
│       ├── premium/
│       │   └── PremiumManager.h/.cpp 
│       └── updater/
│           └── Updater.h/.cpp     
├── tests/
│   ├── CMakeLists.txt             
│   └── test_thakur.cpp            
└── installer/
    └── thakur_installer.iss       
```

---

- Windows 10 / 11 (64-bit)
- Visual Studio 2022 (MSVC v143 C++ x64 build tools)
- Qt 6.4+ (Components: Core, Gui, Widgets, Network, WebEngineWidgets, Multimedia)
- CMake 3.22+ & Ninja

Open PowerShell in the repository root and run:
```powershell
.\build_thakur.ps1 -Config Release
```
The compiled executable and all runtime dependencies deployed by `windeployqt` will be in `build/bin/thakur.exe`.

---

Yes. DLL injection and remote process memory manipulation (`VirtualAllocEx`, `WriteProcessMemory`, `CreateRemoteThread`) require administrator access.

During pre-release testing, entering key **`373438`** in the Key Modal provides an immediate 7-day trial unlocking all features.

The internal heuristic scanner inspects payloads for risky patterns (such as obfuscated bit manipulation, untrusted HTTP calls, or file system modifications) and marks them with visual badges: `Clean`, `Caution`, `Danger`, or `Obfuscated`.
