# Thakur

Thakur is a Windows desktop application with a native Win32 host and a Microsoft WebView2-rendered UI.

## Desktop architecture

- **Native Win32 host:** window creation, lifecycle, Windows integration, IPC, and native application services.
- **Microsoft WebView2:** renders the HTML/CSS/JavaScript interface inside the desktop window.
- **WebView2 bridge:** native C++ and the UI communicate through WebView2 web messages.
- **Web UI:** the project can load the existing HTML prototype without adding a separate UI framework.

## Repository layout

```text
Thakur/
├── CMakeLists.txt
├── build_thakur.bat
├── build_thakur.ps1
├── .github/workflows/windows-build.yml
├── thakur.rc
├── thakur.manifest
├── thakur_ui/
│   ├── resources/
│   │   ├── images/
│   │   └── themes/
│   └── src/
│       ├── main.cpp
│       ├── script_hub.cpp/.h
│       ├── ipc/
│       ├── injector/
│       └── supporting native code
└── tests/
```

## Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2022 C++ build tools
- CMake 3.22+
- Ninja
- Microsoft WebView2 Runtime
- Microsoft.Web.WebView2 SDK (the build scripts can install it into `packages/`)

## Build

### Batch build

```bat
build_thakur.bat
```

### PowerShell/CMake build

```powershell
.\build_thakur.ps1 -Config Release
```

For a clean rebuild:

```powershell
.\build_thakur.ps1 -Config Release -Clean
```

The executable is produced as:

```text
build\bin\thakur.exe
```

`WebView2Loader.dll` is staged beside the executable.

## Notes

`index.html` is kept as a UI prototype. It is not converted into a backend and is not modified by the native build migration.
