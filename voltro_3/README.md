# VOLTRO EXECUTOR (v1.5.0)

High-performance, native Windows-based Roblox script executor designed to provide a modern, secure, and intuitive experience for executing Lua scripts in Roblox games.

---

## Key Features

- **Native Qt6 C++ Interface**: Frameless, modern dark theme with custom neon-green (`#00FF66`) accents, customizable themes (Dark, Midnight, Glitched), and smooth transitions.
- **Advanced Code Editor**: Syntax highlighting for Luau/Roblox APIs, line numbers, multi-tab workspace, auto-indentation, and find/replace.
- **Dynamic Multi-Hub Aggregator**: Search verified scripts from `rscripts.net`, `scripthub.com`, `robloxscripts.com`, and `scriptblox.com` with real-time exploit security heuristics analysis.
- **Stealth Injection & Execution Engine**: Manual mapping & process hollowing evasion, **inj-1 Manual Mapping with NtSetIoCompletion** for advanced stealth injection, dynamic pattern scanning for `lua_State*` and `luaL_dostring`, with Named Pipes (`\.\pipe\VoltroIPC`) for robust IPC.
- **Qt WebEngine Integration**: Full web browser component support for embedded web views and modern UI features.
- **Hardware ID (HWID) & Premium Key System**: Tiered access with offline caching, local encryption, and server-side verification.
- **Background Auto-Updater**: Automated release checks with SHA-256 checksum verification and automatic rollbacks.

---

## Project Structure

```
voltro/
├── CMakeLists.txt                 # Main Qt6 CMake build configuration
├── build_voltro.ps1               # Automated build script (CMake + Ninja)
├── .github/workflows/             # CI/CD Windows workflow
├── voltro_ui/
│   ├── resources.qrc              # Qt compiled resources (icons, themes)
│   ├── resources/
│   │   ├── images/                # Logo and icon assets
│   │   └── themes/                # Theme definitions (dark, midnight, glitched)
│   ├── ui/                        # Qt Designer UI forms
│   └── src/
│       ├── main_qt.cpp            # Application entry point
│       ├── mainwindow.h/.cpp      # Main application window & event loop
│       ├── ThemeManager.h/.cpp    # Dynamic JSON theme loader & stylesheet builder
│       ├── EditorWidget.h/.cpp    # Lua syntax highlighter & code editor widget
│       ├── splashscreen.h/.cpp    # Animated splash screen
│       ├── hub/
│       │   ├── HubManager.h/.cpp  # Multi-hub network aggregator & search
│       │   └── ScriptCard.h/.cpp  # Script presentation widget with safety badges
│       ├── injector/
│       │   ├── VoltroInjector.h/.cpp # Manual mapping & process hollowing engine
│       │   ├── Inj1Injector.h/.cpp # inj-1 Manual Mapping with NtSetIoCompletion
│       │   └── patterns.json      # Memory offset signatures
│       ├── premium/
│       │   └── PremiumManager.h/.cpp # HWID generation & key validation
│       └── updater/
│           └── Updater.h/.cpp     # GitHub releases checker & installer
├── tests/
│   ├── CMakeLists.txt             # Test suite configuration
│   └── test_voltro.cpp            # Unit tests for Theme, Hub, and Premium managers
└── installer/
    └── voltro_installer.iss       # Inno Setup packaging script
```

---

## Building from Source

### Prerequisites
- Windows 10 / 11 (64-bit)
- Visual Studio 2022 (MSVC v143 C++ x64 build tools)
- Qt 6.4+ (Components: Core, Gui, Widgets, Network, WebEngineCore, WebEngineWidgets, WebEngine, Multimedia)
- CMake 3.22+ & Ninja

### Quick Build
Open PowerShell in the repository root and run:
```powershell
.uild_voltro.ps1 -Config Release
```
The compiled executable and all runtime dependencies deployed by `windeployqt` will be in `build/bin/voltro3.exe`.

---

## Frequently Asked Questions (FAQ)

### 1. Does Voltro require administrator privileges?
Yes. DLL injection and remote process memory manipulation (`VirtualAllocEx`, `WriteProcessMemory`, `CreateRemoteThread`, `NtSetIoCompletion`) require administrator access.

### 2. What is the test key?
During pre-release testing, entering key **`373438`** in the Key Modal provides an immediate 7-day trial unlocking all features.

### 3. How does the Script Hub ensure safety?
The internal heuristic scanner inspects payloads for risky patterns (such as obfuscated bit manipulation, untrusted HTTP calls, or file system modifications) and marks them with visual badges: `Clean`, `Caution`, `Danger`, or `Obfuscated`.

### 4. What injection methods are available?
- **Standard Manual Mapping**: Traditional PE mapping into target process
- **inj-1 Manual Mapping**: Advanced technique using NtSetIoCompletion with Thread Pool Worker for maximum stealth (bypasses standard user-mode AC heuristics)
- **Process Hollowing**: Spawns svchost.exe suspended, unmaps its image, replaces with our PE

---

## Injection Comparison (inj-1 vs Traditional)

| Feature | Voltro 3 Current (injector.h) | inj-1 (Inj1Injector.h) |
|---------|-------------------------------|------------------------|
| **Method** | LoadLibraryA / Thread Context Hijacking | Manual Mapping + Section Stomping |
| **Memory Allocation** | VirtualAllocEx (MEM_PRIVATE) - Flagged by AC | Section from legitimate System32 DLL - Appears image-backed |
| **Execution Trigger** | SetThreadContext / CreateRemoteThread | NtSetIoCompletion (Windows ThreadPool worker) |
| **Payload Format** | Plain DLL path on disk | Raw memory PE mapping (can load from bytes/resource) |
| **Anti-Cheat Resistance** | Low (instant detection/crash on current Roblox) | Very High (bypasses standard user-mode AC heuristics) |
