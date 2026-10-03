#Requires -Version 5.1
<#
.SYNOPSIS
  Compila y ejecuta la Demo 2 de Boids en Windows.

.PARAMETER Mode
  build            Solo configurar y compilar
  test             Compilar + ctest
  benchmark        Sin UI: secuencial + dummies (incluye CMP) + frames PPM  [default]
  sequential-ui    Solo ventana grafica secuencial
  cmp-ui           Solo ventana grafica CMP dummy
  all              Tests + benchmark + UI secuencial + UI CMP

.EXAMPLE
  .\proyect_1-Multithreading\scripts\run_demo2.ps1
  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode sequential-ui
  .\proyect_1-Multithreading\scripts\run_demo2.ps1 -Mode all
#>

[CmdletBinding()]
param(
    [ValidateSet("build", "test", "benchmark", "sequential-ui", "cmp-ui", "sequential", "cmp", "all")]
    [string]$Mode = "benchmark",
    [switch]$NoVisualBuild
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
$BoidsSrc = Join-Path $RepoRoot "proyect_1-Multithreading\Boids"
$BuildDir = Join-Path $RepoRoot "build\boids"
$MarkerDir = Join-Path $env:USERPROFILE ".local\share\boids-demo2"
$VsDevCmdFile = Join-Path $MarkerDir "vsdevcmd.txt"

function Write-Info([string]$Message) { Write-Host "==> $Message" -ForegroundColor Cyan }
function Write-Ok([string]$Message)   { Write-Host "OK  $Message" -ForegroundColor Green }

function Refresh-Path {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$user;$machine"

    # MinGW (WinLibs / MSYS2): DLLs runtime para ejecutables enlazados con g++
    $mingwCandidates = @(
        "C:\msys64\ucrt64\bin",
        "C:\msys64\mingw64\bin"
    )
    $wingetRoot = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages"
    if (Test-Path $wingetRoot) {
        Get-ChildItem -Path $wingetRoot -Directory -Filter "BrechtSanders.WinLibs*" -ErrorAction SilentlyContinue |
            ForEach-Object {
                $bin = Join-Path $_.FullName "mingw64\bin"
                if (Test-Path $bin) { $mingwCandidates += $bin }
            }
    }
    foreach ($bin in $mingwCandidates) {
        if ((Test-Path $bin) -and ($env:Path -notlike "*$bin*")) {
            $env:Path = "$bin;$env:Path"
        }
    }
}

function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    foreach ($c in @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\CMake\bin\cmake.exe"
    )) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Enter-VsDevEnvironment {
    $devCmd = $null
    if (Test-Path $VsDevCmdFile) {
        $devCmd = (Get-Content $VsDevCmdFile -Raw).Trim()
    }
    if (-not $devCmd -or -not (Test-Path $devCmd)) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $installPath = & $vswhere -latest -products * `
                -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
                -property installationPath 2>$null
            if ($installPath) {
                $devCmd = Join-Path $installPath "Common7\Tools\VsDevCmd.bat"
            }
        }
    }
    if ($devCmd -and (Test-Path $devCmd)) {
        Write-Info "Cargando entorno MSVC: $devCmd"
        $tempCmd = Join-Path $env:TEMP "boids-vsenv.cmd"
        @"
@echo off
call "$devCmd" -arch=x64 -host_arch=x64 >nul
set
"@ | Set-Content -Path $tempCmd -Encoding ASCII
        $vars = & cmd /c "`"$tempCmd`""
        foreach ($line in $vars) {
            if ($line -match "^(.*?)=(.*)$") {
                Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
            }
        }
        return $true
    }
    return $false
}

function Ensure-Toolchain {
    Refresh-Path
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "Falta git. Ejecute primero: .\proyect_1-Multithreading\scripts\setup_windows.ps1"
    }
    $cmake = Find-CMake
    if (-not $cmake) {
        throw "Falta cmake. Ejecute primero: .\proyect_1-Multithreading\scripts\setup_windows.ps1"
    }

    $hasCl = [bool](Get-Command cl -ErrorAction SilentlyContinue)
    $hasGxx = [bool](Get-Command g++ -ErrorAction SilentlyContinue)
    if (-not $hasCl) {
        [void](Enter-VsDevEnvironment)
        $hasCl = [bool](Get-Command cl -ErrorAction SilentlyContinue)
    }
    if (-not $hasCl -and -not $hasGxx) {
        foreach ($bin in @("C:\msys64\ucrt64\bin", "C:\msys64\mingw64\bin")) {
            if (Test-Path $bin) { $env:Path = "$bin;$env:Path" }
        }
        $hasGxx = [bool](Get-Command g++ -ErrorAction SilentlyContinue)
    }
    if (-not $hasCl -and -not $hasGxx) {
        throw "No hay compilador C++ (cl/g++). Ejecute setup_windows.ps1"
    }
    return $cmake
}

function Invoke-ConfigureAndBuild([string]$CMakeExe) {
    $visual = if ($NoVisualBuild) { "OFF" } else { "ON" }
    Write-Info "Configurando CMake ($BuildDir), BOIDS_BUILD_VISUAL=$visual"

    $generatorArgs = @()
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        $generatorArgs = @()
    } elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
        $generatorArgs = @("-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release")
    }

    & $CMakeExe -S $BoidsSrc -B $BuildDir @generatorArgs "-DBOIDS_BUILD_VISUAL=$visual"
    if ($LASTEXITCODE -ne 0) { throw "cmake configure fallo" }

    Write-Info "Compilando Release..."
    & $CMakeExe --build $BuildDir --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "cmake build fallo" }
}

function Find-Exe([string]$Name) {
    $candidates = @(
        (Join-Path $BuildDir "$Name.exe"),
        (Join-Path $BuildDir "Release\$Name.exe"),
        (Join-Path $BuildDir "Debug\$Name.exe"),
        (Join-Path $BuildDir $Name)
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    return $null
}

function Invoke-Tests([string]$CMakeExe) {
    Write-Info "Ejecutando ctest..."
    $ctestCmd = Get-Command ctest -ErrorAction SilentlyContinue
    if (-not $ctestCmd) {
        $ctestPath = Join-Path (Split-Path $CMakeExe -Parent) "ctest.exe"
        if (Test-Path $ctestPath) {
            & $ctestPath --test-dir $BuildDir -C Release --output-on-failure
        } else {
            throw "ctest no encontrado"
        }
    } else {
        & ctest --test-dir $BuildDir -C Release --output-on-failure
    }
    if ($LASTEXITCODE -ne 0) { throw "ctest fallo" }
}

function Find-Boids {
    $exe = Find-Exe "boids"
    if (-not $exe) { throw "No se encontro boids.exe. Compile primero (-Mode build)." }
    return $exe
}

function Invoke-Benchmark {
    $exe = Find-Boids
    Write-Info "Demo2 compare sin UI: --scheme compare"
    Push-Location $RepoRoot
    try {
        & $exe --scheme compare
        if ($LASTEXITCODE -ne 0) { throw "boids --scheme compare fallo" }
    } finally {
        Pop-Location
    }
    Write-Ok "Frames en: $(Join-Path $RepoRoot 'frames')"
}

function Invoke-Visual([string]$Scheme) {
    $exe = Find-Boids
    Write-Info "UI: boids --scheme $Scheme --gui (cierre la ventana para continuar)"
    & $exe --scheme $Scheme --gui
    if ($LASTEXITCODE -ne 0) { throw "boids --gui fallo" }
}

function Invoke-HeadlessScheme([string]$Scheme, [switch]$Forever) {
    $exe = Find-Boids
    $args = @("--scheme", $Scheme, "--no-gui")
    if ($Forever) { $args += "--forever" } else { $args += @("--steps", "100") }
    Write-Info ("Headless: boids " + ($args -join " "))
    Push-Location $RepoRoot
    try {
        & $exe @args
        if ($LASTEXITCODE -ne 0) { throw "boids headless fallo" }
    } finally {
        Pop-Location
    }
}

Write-Info "Demo 2 Boids - modo: $Mode"
$cmake = Ensure-Toolchain

switch ($Mode) {
    "build" {
        Invoke-ConfigureAndBuild $cmake
    }
    "test" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-Tests $cmake
    }
    "benchmark" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-Tests $cmake
        Invoke-Benchmark
    }
    "sequential-ui" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-Visual "sequential"
    }
    "cmp-ui" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-Visual "cmp"
    }
    "sequential" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-HeadlessScheme "sequential" -Forever
    }
    "cmp" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-HeadlessScheme "cmp" -Forever
    }
    "all" {
        Invoke-ConfigureAndBuild $cmake
        Invoke-Tests $cmake
        Invoke-Benchmark
        Write-Info "UI secuencial (cierre la ventana para seguir)..."
        try { Invoke-Visual "sequential" } catch { Write-Host $_ -ForegroundColor Yellow }
        Write-Info "UI CMP dummy..."
        try { Invoke-Visual "cmp" } catch { Write-Host $_ -ForegroundColor Yellow }
    }
}

Write-Ok "Listo ($Mode)."