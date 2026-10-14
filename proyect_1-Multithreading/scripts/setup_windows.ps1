#Requires -Version 5.1
<#
.SYNOPSIS
  Instala y agrega al PATH las dependencias para Boids (Demo 2) en Windows.

.DESCRIPTION
  Detecta e instala (si faltan):
    - CMake >= 3.16
    - Compilador C++17 (Visual Studio Build Tools preferido; MinGW via MSYS2 como fallback)
    - Git (necesario para FetchContent de Raylib)

  Uso (PowerShell como usuario normal; pedira elevacion solo si hace falta):
    Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
    .\proyect_1-Multithreading\scripts\setup_windows.ps1

  Luego abra una terminal nueva y ejecute:
    .\proyect_1-Multithreading\scripts\run_demo2.ps1
#>

[CmdletBinding()]
param(
    [switch]$SkipRaylibHint
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
$LocalBin = Join-Path $env:USERPROFILE ".local\bin"
$MarkerDir = Join-Path $env:USERPROFILE ".local\share\boids-demo2"
$PathMarker = Join-Path $MarkerDir "path.txt"

function Write-Info([string]$Message) { Write-Host "==> $Message" -ForegroundColor Cyan }
function Write-Warn([string]$Message) { Write-Host "!!  $Message" -ForegroundColor Yellow }
function Write-Ok([string]$Message)   { Write-Host "OK  $Message" -ForegroundColor Green }

function Test-Command([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-Directory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

function Add-ToUserPath([string]$Directory) {
    if (-not (Test-Path $Directory)) { return }
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $userPath) { $userPath = "" }
    $parts = $userPath -split ";" | Where-Object { $_ -and $_.Trim() -ne "" }
    $normalized = $Directory.TrimEnd("\")
    $already = $parts | Where-Object { $_.TrimEnd("\") -ieq $normalized }
    if (-not $already) {
        $newPath = if ($userPath.Trim().Length -eq 0) { $normalized } else { "$normalized;$userPath" }
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        Write-Ok "Anadido al PATH de usuario: $normalized"
    } else {
        Write-Info "Ya estaba en PATH de usuario: $normalized"
    }
    if ($env:Path -notlike "*$normalized*") {
        $env:Path = "$normalized;$env:Path"
    }
}

function Invoke-WingetInstall([string]$Id, [string]$DisplayName) {
    if (-not (Test-Command "winget")) {
        throw "winget no esta disponible. Instale 'App Installer' desde Microsoft Store."
    }
    Write-Info "Instalando $DisplayName ($Id) con winget..."
    $args = @(
        "install", "--id", $Id,
        "-e", "--accept-package-agreements", "--accept-source-agreements",
        "--disable-interactivity"
    )
    & winget @args
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne -1978335189) {
        Write-Warn "winget devolvio codigo $LASTEXITCODE para $DisplayName (puede ya estar instalado)."
    }
}

function Refresh-PathFromMachine {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$user;$machine"
}

function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\CMake\bin\cmake.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Find-VsDevCmd {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }
    $installPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null
    if (-not $installPath) { return $null }
    $devCmd = Join-Path $installPath "Common7\Tools\VsDevCmd.bat"
    if (Test-Path $devCmd) { return $devCmd }
    return $null
}

function Find-MingwGxx {
    $cmd = Get-Command g++ -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        "C:\msys64\ucrt64\bin\g++.exe",
        "C:\msys64\mingw64\bin\g++.exe",
        "C:\mingw64\bin\g++.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Ensure-Git {
    if (Test-Command "git") {
        Write-Ok "Git: $(git --version)"
        return
    }
    Invoke-WingetInstall -Id "Git.Git" -DisplayName "Git"
    Refresh-PathFromMachine
    Add-ToUserPath "C:\Program Files\Git\cmd"
    if (-not (Test-Command "git")) {
        throw "Git no quedo disponible en PATH. Cierre y abra PowerShell e intente de nuevo."
    }
    Write-Ok "Git: $(git --version)"
}

function Ensure-CMake {
    $cmake = Find-CMake
    if ($cmake) {
        Add-ToUserPath (Split-Path $cmake -Parent)
        $ver = (& $cmake --version | Select-Object -First 1)
        Write-Ok "CMake: $ver ($cmake)"
        return
    }
    Invoke-WingetInstall -Id "Kitware.CMake" -DisplayName "CMake"
    Refresh-PathFromMachine
    Add-ToUserPath "C:\Program Files\CMake\bin"
    $cmake = Find-CMake
    if (-not $cmake) {
        throw "CMake no quedo disponible. Abra una terminal nueva y reintente."
    }
    Write-Ok "CMake instalado: $cmake"
}

function Ensure-Compiler {
    $devCmd = Find-VsDevCmd
    if ($devCmd) {
        Write-Ok "Visual C++ / Build Tools detectado: $devCmd"
        Ensure-Directory $MarkerDir
        Set-Content -Path (Join-Path $MarkerDir "vsdevcmd.txt") -Value $devCmd -Encoding ASCII
        return "msvc"
    }

    $gxx = Find-MingwGxx
    if ($gxx) {
        $bin = Split-Path $gxx -Parent
        Add-ToUserPath $bin
        Write-Ok "MinGW g++: $gxx"
        return "mingw"
    }

    Write-Info "No hay compilador C++. Intentando Visual Studio 2022 Build Tools..."
    try {
        Invoke-WingetInstall -Id "Microsoft.VisualStudio.2022.BuildTools" `
            -DisplayName "VS 2022 Build Tools"
        if (Test-Command "winget") {
            Write-Info "Asegurando workload Microsoft.VisualStudio.Workload.VCTools..."
            & winget install --id Microsoft.VisualStudio.2022.BuildTools -e `
                --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" `
                --accept-package-agreements --accept-source-agreements --disable-interactivity
        }
    } catch {
        Write-Warn "No se pudo instalar Build Tools automaticamente: $_"
    }

    Refresh-PathFromMachine
    $devCmd = Find-VsDevCmd
    if ($devCmd) {
        Ensure-Directory $MarkerDir
        Set-Content -Path (Join-Path $MarkerDir "vsdevcmd.txt") -Value $devCmd -Encoding ASCII
        Write-Ok "Build Tools listo: $devCmd"
        return "msvc"
    }

    Write-Info "Fallback: instalando MSYS2 + MinGW UCRT64..."
    Invoke-WingetInstall -Id "MSYS2.MSYS2" -DisplayName "MSYS2"
    $bash = "C:\msys64\usr\bin\bash.exe"
    if (-not (Test-Path $bash)) {
        throw "MSYS2 no se instalo correctamente."
    }
    Write-Info "Instalando toolchain MinGW (puede tardar)..."
    & $bash -lc "pacman -Sy --noconfirm mingw-w64-ucrt64-gcc mingw-w64-ucrt64-cmake make"
    Add-ToUserPath "C:\msys64\ucrt64\bin"
    Refresh-PathFromMachine
    $gxx = Find-MingwGxx
    if (-not $gxx) {
        throw "No se encontro un compilador C++ tras el setup. Instale VS Build Tools o MSYS2 manualmente."
    }
    Write-Ok "MinGW g++: $gxx"
    return "mingw"
}

Write-Info "Setup Windows - Boids Demo 2"
Write-Info "Repositorio: $RepoRoot"

Ensure-Directory $LocalBin
Ensure-Directory $MarkerDir
Add-ToUserPath $LocalBin

Ensure-Git
Ensure-CMake
$toolchain = Ensure-Compiler

Set-Content -Path $PathMarker -Value @"
toolchain=$toolchain
repo=$RepoRoot
local_bin=$LocalBin
"@ -Encoding ASCII

if (-not $SkipRaylibHint) {
    Write-Info "Raylib: CMake lo descargara con FetchContent al compilar boids_visual (requiere red)."
}

Write-Host ""
Write-Ok "Setup completado (toolchain=$toolchain)."
Write-Info "Cierre y abra PowerShell (o recargue el PATH) y ejecute:"
Write-Host "  .\proyect_1-Multithreading\scripts\run_demo2.ps1" -ForegroundColor White
Write-Host "Opciones utiles:" -ForegroundColor Gray
Write-Host "  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode all" -ForegroundColor Gray
Write-Host "  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode sequential-ui" -ForegroundColor Gray
Write-Host "  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode cmp-ui" -ForegroundColor Gray
Write-Host "  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode benchmark" -ForegroundColor Gray