# Voltro 2: Solara Architecture Edition (RBLX x64)

`voltro_2` is a complete, self-contained executor package consisting of:
1. **`voltro2.exe`**: The standalone Win32 + Microsoft WebView2 desktop application with the complete Voltro UI (Monaco Editor, Script Hub, Themes, Key System, and Console).
2. **`voltro2_runtime.dll`**: The injected Luau execution backend built on the **Solara Architecture** for Roblox.
3. **`ui/index.html`**: The complete WebView2 user interface bundle.

---

## 🏛️ Architecture Highlights

### 1. Standalone Application (`voltro2.exe`)
- Native Win32 desktop window with frameless modern UI, DWM dark mode, rounded corners, and drop shadows.
- Embedded Microsoft WebView2 hosting Monaco Editor with Lua/Luau syntax highlighting.
- Built-in hotkeys: `WIN + T` (New Tab), `WIN + W` (Close Tab).
- Connects directly to the `voltro2_runtime.dll` backend over Named Pipe IPC (`\\.\pipe\VoltroIPC`).

### 2. TaskScheduler Synchronization & Main-Thread Stepping
- Hooks Roblox's engine **TaskScheduler** (`WaitingHybridScriptsJob` / `Heartbeat`).
- Scripts are held in a thread-safe queue and **executed synchronously on the Roblox main game thread tick**.

### 3. Luau State & Capability Elevation
- Scans for active `ScriptContext` / `RobloxExtraSpace`.
- Dynamically elevates capabilities to `CAPABILITIES_ALL` (`0x3FFFFFFFFFFFFFFFULL`) and identity to `8`.

### 4. SEH Crash-Proof Protection
- All script evaluations and VTable hooks are protected by isolated SEH barriers (`__try / __except`).
- Any user script error or memory fault is caught and forwarded to the console—**the Roblox client never crashes**.

---

## 📁 Directory Structure

```text
voltro_2/
├── include/
│   └── solara/
│       ├── solara.h          # Core Luau types, identity bitmasks, memory safety
│       ├── injector.h        # MSVC x64 compliant trampoline and remote injector
│       ├── solara_engine.h   # TaskScheduler scanner, VTable hook, main-thread stepper
│       ├── solara_ipc.h      # Named Pipe client/server bridge
│       └── watchdog.h        # Process supervisor and crash watchdog
├── src/
│   ├── main_app.cpp          # Standalone UI Application Host (voltro2.exe)
│   ├── solara_engine.cpp     # Engine implementation with SEH safety
│   ├── ipc_client.cpp        # Full Voltro IPC bridge & JSON protocol handler
│   ├── watchdog.cpp          # Real-time process watchdog
│   └── voltro2_runtime.cpp   # Injected DLL entry point (DllMain & C exports)
├── ui/
│   └── index.html            # Complete Voltro UI (Monaco + Script Hub)
├── tests/
│   └── test_runtime.cpp      # Standalone verification test harness
├── build/
│   ├── voltro2.exe           # Standalone Application Executable
│   ├── voltro2_runtime.dll   # Solara Injected Runtime DLL
│   ├── WebView2Loader.dll    # WebView2 Runtime Loader
│   ├── ui/index.html         # Staged UI Bundle
│   └── test_runtime.exe      # Verification Test Executable
├── build.bat                 # One-click MSVC x64 build script
└── README.md                 # Full documentation
```

---

## 🚀 How to Launch Voltro 2

Simply run the standalone executable:
```cmd
voltro_2\build\voltro2.exe
```

Or rebuild everything from scratch at any time:
```cmd
voltro_2\build.bat
```
