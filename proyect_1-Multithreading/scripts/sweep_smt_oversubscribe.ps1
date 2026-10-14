#Requires -Version 5.1
<#
.SYNOPSIS
  Barrido de --oversubscribe para la aproximación SMT de Boids.

.EXAMPLE
  .\proyect_1-Multithreading\scripts\sweep_smt_oversubscribe.ps1
  .\proyect_1-Multithreading\scripts\sweep_smt_oversubscribe.ps1 -Boids 200 -Steps 50
#>
[CmdletBinding()]
param(
    [string]$Binary = "",
    [int]$Boids = 200,
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

Write-Host "bin=$Binary boids=$Boids steps=$Steps seed=$Seed"
foreach ($F in @(1, 2, 4, 8)) {
    Write-Host "==> smt oversubscribe=$F" -ForegroundColor Cyan
    & $Binary --scheme smt --oversubscribe $F --no-gui `
        --boids $Boids --steps $Steps --seed $Seed --validate
    if ($LASTEXITCODE -ne 0) {
        throw "Falló SMT con oversubscribe=$F (exit $LASTEXITCODE)"
    }
}
Write-Host "Barrido SMT OK" -ForegroundColor Green
