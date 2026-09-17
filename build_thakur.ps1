[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [switch]$Clean,
    [switch]$NoBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Info([string]$Message) { Write-Host "[INFO] $Message" -ForegroundColor Cyan }
function Ok([string]$Message)   { Write-Host "[ OK ] $Message" -ForegroundColor Green }
function Err([string]$Message)  { Write-Host "[ERR ] $Message" -ForegroundColor Red }

$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"
$BinDir = Join-Path $BuildDir "bin"
$PackagesDir = Join-Path $ProjectRoot "packages"

Info "Thakur Native Win32 + Microsoft WebView2 build"
Info "Project root: $ProjectRoot"
Info "Configuration: $Config"

foreach ($tool in @("cmake", "cl")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Err "$tool was not found in PATH. Open a Visual Studio Developer PowerShell/Command Prompt and retry."
        exit 1
    }
}

if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    $ninja = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Filter "ninja.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($ninja) {
        $env:PATH = "$(Split-Path $ninja.FullName -Parent);$env:PATH"
    } else {
        Err "Ninja was not found. Install Ninja and add it to PATH."
        exit 1
    }
}

if ($Clean -and (Test-Path $BuildDir)) {
    Info "Removing build directory..."
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $PackagesDir | Out-Null

$webViewPackage = Get-ChildItem $PackagesDir -Directory -Filter "Microsoft.Web.WebView2.*" -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending |
    Select-Object -First 1

if (-not $webViewPackage) {
    Info "Microsoft.Web.WebView2 SDK not found locally."

    $nugetCommand = Get-Command nuget -ErrorAction SilentlyContinue
    if ($nugetCommand) {
        $nugetExe = $nugetCommand.Source
    } else {
        $nugetExe = Join-Path $ProjectRoot "nuget.exe"
        Info "Downloading NuGet CLI..."
        Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile $nugetExe
    }

    Info "Installing Microsoft.Web.WebView2 SDK..."
    & $nugetExe install Microsoft.Web.WebView2 -OutputDirectory $PackagesDir -NonInteractive -NoCache
    if ($LASTEXITCODE -ne 0) {
        Err "WebView2 SDK installation failed."
        exit $LASTEXITCODE
    }

    $webViewPackage = Get-ChildItem $PackagesDir -Directory -Filter "Microsoft.Web.WebView2.*" |
        Sort-Object Name -Descending |
        Select-Object -First 1
}

if (-not $webViewPackage) {
    Err "Microsoft.Web.WebView2 package was not found in $PackagesDir."
    exit 1
}

$webViewRoot = $webViewPackage.FullName
Info "WebView2 SDK: $webViewRoot"

$cmakeArgs = @(
    "-S", $ProjectRoot,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DWEBVIEW2_SDK_ROOT=$webViewRoot",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$BinDir"
)

Info "Configuring CMake..."
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($NoBuild) {
    Ok "Configure completed."
    exit 0
}

Info "Building..."
cmake --build $BuildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$ExePath = Join-Path $BinDir "thakur.exe"
$LoaderPath = Join-Path $BinDir "WebView2Loader.dll"

if (-not (Test-Path $ExePath)) {
    Err "Expected executable not found: $ExePath"
    exit 1
}
if (-not (Test-Path $LoaderPath)) {
    Err "Expected WebView2Loader.dll not found: $LoaderPath"
    exit 1
}

Ok "Build successful."
Write-Host "Executable: $ExePath"
Write-Host "WebView2:   $LoaderPath"
