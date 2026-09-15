<
.SYNOPSIS
    Build script for Thakur Executor (Qt6 / CMake / Ninja).

.DESCRIPTION
    Automatically locates Qt6, configures CMake with Ninja, builds the project,
    and runs windeployqt6 on the output executable.

.PARAMETER Config
    Build configuration: Debug or Release (default: Release).

.PARAMETER Clean
    If specified, deletes the build/ directory before configuring.

.PARAMETER NoBuild
    If specified, only runs CMake configure; does not compile.

.EXAMPLE
    .\build_thakur.ps1
    .\build_thakur.ps1 -Config Debug -Clean
    .\build_thakur.ps1 -NoBuild

[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",

    [switch]$Clean,
    [switch]$NoBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Success { param([string]$Msg) Write-Host "[OK]  $Msg" -ForegroundColor Green  }
function Write-Warn    { param([string]$Msg) Write-Host "[WARN] $Msg" -ForegroundColor Yellow }
function Write-Err     { param([string]$Msg) Write-Host "[ERR] $Msg" -ForegroundColor Red    }
function Write-Info    { param([string]$Msg) Write-Host "[..] $Msg"  -ForegroundColor Cyan   }

function Find-Qt6Root {
    $candidateRoots = @(
        "C:\Qt",
        "D:\Qt",
        "$env:USERPROFILE\Qt",
        "C:\Program Files\Qt",
        "C:\Program Files (x86)\Qt"
    )

    
    $archSubs = @(
        "msvc2022_64",
        "msvc2019_64",
        "msvc2022_arm64",
        "llvm-mingw_64",
        "mingw_64"
    )

    foreach ($root in $candidateRoots) {
        if (-not (Test-Path $root)) { continue }

        
        $versionDirs = Get-ChildItem -Path $root -Directory |
            Where-Object { $_.Name -match "^6\.\d+\.\d+$" } |
            Sort-Object Name -Descending

        foreach ($vDir in $versionDirs) {
            foreach ($arch in $archSubs) {
                $candidate = Join-Path $vDir.FullName $arch
                $cmakeDir  = Join-Path $candidate "lib\cmake\Qt6"
                if (Test-Path $cmakeDir) {
                    return $candidate
                }
            }
        }
    }

    
    if ($env:Qt6_DIR -and (Test-Path $env:Qt6_DIR)) {
        
        return (Split-Path (Split-Path $env:Qt6_DIR -Parent) -Parent)
    }

    return $null
}

$ProjectRoot = $PSScriptRoot
$BuildDir    = Join-Path $ProjectRoot "build"
$BinDir      = Join-Path $BuildDir "bin"

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Magenta
Write-Host "║          Thakur Executor Build Script            ║" -ForegroundColor Magenta
Write-Host "║                  Version 1.5.0                  ║" -ForegroundColor Magenta
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Magenta
Write-Host ""

Write-Info "Project root : $ProjectRoot"
Write-Info "Build dir    : $BuildDir"
Write-Info "Configuration: $Config"
Write-Host ""

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Info "Cleaning build directory..."
    Remove-Item -Recurse -Force $BuildDir
    Write-Success "Build directory removed."
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Err "cmake not found in PATH. Install CMake 3.22+ and add it to PATH."
    exit 1
}
Write-Info "CMake: $($cmake.Source)"

$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if (-not $ninja) {
    Write-Warn "ninja not found in PATH. Attempting to find it..."
    
    $vsBase = "C:\Program Files\Microsoft Visual Studio"
    $ninjaPath = Get-ChildItem -Path $vsBase -Recurse -Filter "ninja.exe" -ErrorAction SilentlyContinue |
                 Select-Object -First 1 -ExpandProperty FullName
    if ($ninjaPath) {
        $env:PATH = "$([System.IO.Path]::GetDirectoryName($ninjaPath));$env:PATH"
        Write-Info "Found Ninja at: $ninjaPath"
    } else {
        Write-Err "Ninja not found. Install Ninja (https:
        exit 1
    }
} else {
    Write-Info "Ninja: $($ninja.Source)"
}

$qtRoot = Find-Qt6Root
if (-not $qtRoot) {
    if ($env:CMAKE_PREFIX_PATH) {
        Write-Warn "Qt6 not auto-detected but CMAKE_PREFIX_PATH is set: $env:CMAKE_PREFIX_PATH"
        $qtRoot = $env:CMAKE_PREFIX_PATH
    } else {
        Write-Err "Qt6 installation not found. Please:"
        Write-Err "  1. Install Qt6 from https:
        Write-Err "  2. Or set the Qt6_DIR environment variable to <Qt>/lib/cmake/Qt6"
        Write-Err "  3. Or set CMAKE_PREFIX_PATH to the Qt install directory"
        exit 1
    }
}
Write-Info "Qt6 root     : $qtRoot"

$qtBin          = Join-Path $qtRoot "bin"
$windeployqt    = $null
$deployqtNames  = @("windeployqt6.exe", "windeployqt.exe")
foreach ($name in $deployqtNames) {
    $candidate = Join-Path $qtBin $name
    if (Test-Path $candidate) { $windeployqt = $candidate; break }
}
if ($windeployqt) {
    Write-Info "windeployqt  : $windeployqt"
} else {
    Write-Warn "windeployqt6 not found in $qtBin — DLL deployment will be skipped."
}

Write-Host ""

if (-not $env:VSCMD_ARG_TGT_ARCH) {
    Write-Info "Initializing MSVC environment..."
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
        if ($vsInstallPath) {
            $vcvarsall = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvarsall.bat"
            if (Test-Path $vcvarsall) {
                Write-Info "Sourcing vcvarsall.bat (amd64)..."
                
                $tempFile = [System.IO.Path]::GetTempFileName() + ".bat"
                "@echo off`ncall `"$vcvarsall`" amd64`nset" | Set-Content $tempFile
                $envVars = cmd /c $tempFile 2>$null
                Remove-Item $tempFile -ErrorAction SilentlyContinue
                foreach ($line in $envVars) {
                    if ($line -match "^([^=]+)=(.*)$") {
                        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
                    }
                }
                Write-Success "MSVC environment loaded."
            }
        }
    } else {
        Write-Warn "vswhere.exe not found. Assuming compiler is already in PATH."
    }
}

Write-Info "Configuring CMake..."
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$cmakeArgs = @(
    "-S", $ProjectRoot,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DCMAKE_PREFIX_PATH=$qtRoot",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$BinDir"
)

Write-Host "  cmake $($cmakeArgs -join ' ')" -ForegroundColor DarkGray
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Err "CMake configuration failed (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}
Write-Success "CMake configuration complete."
Write-Host ""

if ($NoBuild) {
    Write-Warn "-NoBuild specified. Skipping compilation."
    Write-Success "Configure-only run complete. Build files in: $BuildDir"
    exit 0
}

Write-Info "Building Thakur ($Config)..."
$buildArgs = @(
    "--build", $BuildDir,
    "--config", $Config,
    "--parallel"
)

Write-Host "  cmake $($buildArgs -join ' ')" -ForegroundColor DarkGray
& cmake @buildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Err "Build failed (exit code $LASTEXITCODE)."
    exit $LASTEXITCODE
}
Write-Success "Build complete."
Write-Host ""

$exePath = Join-Path $BinDir "thakur.exe"
if (-not (Test-Path $exePath)) {
    
    $found = Get-ChildItem -Path $BuildDir -Filter "thakur.exe" -Recurse -ErrorAction SilentlyContinue |
             Select-Object -First 1 -ExpandProperty FullName
    if ($found) { $exePath = $found }
}

if (-not (Test-Path $exePath)) {
    Write-Warn "thakur.exe not found at expected path: $exePath"
    Write-Warn "Build may have placed it elsewhere. Check $BuildDir"
} else {
    Write-Info "Executable: $exePath"
}

if ($windeployqt -and (Test-Path $exePath)) {
    Write-Info "Running windeployqt6..."
    $deployArgs = @(
        "--no-translations",
        "--no-system-d3d-compiler",
        "--no-opengl-sw",
        "--compiler-runtime",
        $exePath
    )
    & $windeployqt @deployArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Warn "windeployqt6 exited with code $LASTEXITCODE (non-fatal)."
    } else {
        Write-Success "windeployqt6 deployment complete."
    }
} elseif (-not $windeployqt) {
    Write-Warn "Skipping windeployqt (not found). Manually copy Qt DLLs if needed."
}

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║              Build Successful!                  ║" -ForegroundColor Green
Write-Host "╚══════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
if (Test-Path $exePath) {
    Write-Host "  Output: " -NoNewline -ForegroundColor White
    Write-Host $exePath -ForegroundColor Yellow
}
Write-Host ""
