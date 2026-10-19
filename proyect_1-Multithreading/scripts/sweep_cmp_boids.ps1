#Requires -Version 5.1
<#
.SYNOPSIS
  Barrido de --boids para CMP (T = L automático).

.EXAMPLE
  .\proyect_1-Multithreading\scripts\sweep_cmp_boids.ps1
  .\proyect_1-Multithreading\scripts\sweep_cmp_boids.ps1 -Steps 50 -Seed 7
#>
[CmdletBinding()]
param(
    [string]$Binary = "",
    [int]$Steps = 50,
    [int]$Seed = 42
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path

if (-not $Binary) {
    $candidates = @(
        (Join-Path $RepoRoot "build\boids\boids.exe"),
        (Join-Path $RepoRoot "build\boids\Release\boids.exe"),
        (Join-Path $RepoRoot "build\boids\Debug\boids.exe")
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $Binary = $c; break }
    }
}

if (-not $Binary -or -not (Test-Path $Binary)) {
    throw "No se encontró boids.exe. Compile primero (cmake --build build/boids)."
}

Write-Host "bin=$Binary steps=$Steps seed=$Seed (scheme=cmp, T=L)"
foreach ($N in @(50, 100, 200, 400)) {
    Write-Host "==> cmp boids=$N" -ForegroundColor Cyan
    & $Binary --scheme cmp --no-gui `
        --boids $N --steps $Steps --seed $Seed --validate
    if ($LASTEXITCODE -ne 0) {
        throw "Falló CMP con boids=$N (exit $LASTEXITCODE)"
    }
}
Write-Host "Barrido CMP OK" -ForegroundColor Green
