#Requires -Version 5.1
<#
.SYNOPSIS
  Build the unified multithreading tree and run a comparable campaign across
  Boids, Liquids, Ray Tracing, and N-Body.

.DESCRIPTION
  Configures proyect_1-Multithreading/CMakeLists.txt once, then runs one
  process per (problem, workload, workers) cell so memory/state cannot leak
  across problems. Merges summary/raw CSVs into campaign_summary_all.csv and
  campaign_raw_all.csv with a leading "problem" column.

  Sequential is measured once per workload (workers=1) and those rows are
  replicated into every worker cell for that workload, avoiding redundant
  sequential runs while keeping CSVs comparable across workers.

.EXAMPLE
  .\proyect_1-Multithreading\scripts\campaign_all.ps1 -Runs 2 -Warmup 1 -Workloads 100 -Workers 2
  .\proyect_1-Multithreading\scripts\campaign_all.ps1 -Problems boids,liquids -SkipBuild
#>

[CmdletBinding()]
param(
    [int]$Runs = 200,
    [int]$Warmup = 5,
    [int]$Steps = 10,
    [int[]]$Workers = @(2, 4, 8),
    [int[]]$Workloads = @(100, 1000, 10000),
    [int]$SmtFactor = 2,
    [int]$Seed = 42,
    [int]$Quantum = 8,
    [string]$OutputDir = "",
    [string]$BuildDir = "",
    [string]$Problems = "boids,liquids,ray,nbody",
    [string]$RaylibRoot = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

if ($Runs -le 0 -or $Steps -le 0) {
    throw "Runs and Steps must be positive."
}
if ($Warmup -lt 0) {
    throw "Warmup must be >= 0."
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$P1Root = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$RepoRoot = (Resolve-Path (Join-Path $P1Root "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build\p1"
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $stamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
    $OutputDir = Join-Path $P1Root "data\campaign\$stamp"
}

$ProblemSet = @{}
foreach ($token in ($Problems -split ",")) {
    $name = $token.Trim().ToLowerInvariant()
    if ($name) { $ProblemSet[$name] = $true }
}

function Write-Info([string]$Message) { Write-Host "==> $Message" -ForegroundColor Cyan }

function Refresh-Path {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$user;$machine"
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

# Resolves an executable produced by the unified build tree.
function Find-Exe([string]$Name) {
    $candidates = @(
        (Join-Path $BuildDir $Name),
        (Join-Path $BuildDir "$Name.exe"),
        (Join-Path $BuildDir "Release\$Name.exe"),
        (Join-Path $BuildDir "Debug\$Name.exe"),
        (Join-Path $BuildDir "Boids\$Name"),
        (Join-Path $BuildDir "Boids\$Name.exe"),
        (Join-Path $BuildDir "Boids\Release\$Name.exe"),
        (Join-Path $BuildDir "liquids\$Name"),
        (Join-Path $BuildDir "liquids\$Name.exe"),
        (Join-Path $BuildDir "liquids\Release\$Name.exe"),
        (Join-Path $BuildDir "ray_traycing\$Name"),
        (Join-Path $BuildDir "ray_traycing\$Name.exe"),
        (Join-Path $BuildDir "ray_traycing\Release\$Name.exe"),
        (Join-Path $BuildDir "n_body\$Name"),
        (Join-Path $BuildDir "n_body\$Name.exe"),
        (Join-Path $BuildDir "n_body\Release\$Name.exe")
    )
    foreach ($path in $candidates) {
        # Prefer leaf files: $BuildDir/n_body is a directory, not the binary.
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return (Resolve-Path -LiteralPath $path).Path
        }
    }
    return $null
}

# Maps campaign workload N to Ray Tracing width x height.
function Get-RayResolution([int]$Workload) {
    switch ($Workload) {
        100    { return @{ Width = 160; Height = 120 } }
        1000   { return @{ Width = 240; Height = 160 } }
        10000  { return @{ Width = 480; Height = 320 } }
        default {
            $side = [Math]::Max(16, [int][Math]::Sqrt($Workload))
            return @{ Width = $side; Height = $side }
        }
    }
}

# Prepends a problem column to every data row and appends into a merged CSV.
function Merge-CsvWithProblem {
    param(
        [string]$SourcePath,
        [string]$Problem,
        [string]$DestPath,
        [ref]$HeaderWritten
    )
    if (-not (Test-Path $SourcePath)) { return }
    $lines = Get-Content -Path $SourcePath
    if ($lines.Count -eq 0) { return }
    if (-not $HeaderWritten.Value) {
        Set-Content -Path $DestPath -Value ("problem," + $lines[0])
        $HeaderWritten.Value = $true
    }
    for ($i = 1; $i -lt $lines.Count; $i++) {
        if ([string]::IsNullOrWhiteSpace($lines[$i])) { continue }
        Add-Content -Path $DestPath -Value ("$Problem," + $lines[$i])
    }
}

# Concatenates CSV files: header from the first non-empty file, then all data rows.
function Merge-CsvFiles {
    param(
        [string]$DestPath,
        [string[]]$SourcePaths
    )
    $headerWritten = $false
    foreach ($src in $SourcePaths) {
        if (-not (Test-Path -LiteralPath $src)) { continue }
        $lines = Get-Content -LiteralPath $src
        if ($lines.Count -eq 0) { continue }
        if (-not $headerWritten) {
            Set-Content -LiteralPath $DestPath -Value $lines[0]
            $headerWritten = $true
        }
        for ($i = 1; $i -lt $lines.Count; $i++) {
            if ([string]::IsNullOrWhiteSpace($lines[$i])) { continue }
            Add-Content -LiteralPath $DestPath -Value $lines[$i]
        }
    }
    if (-not $headerWritten) {
        throw "Merge-CsvFiles: no source CSVs found for $DestPath"
    }
}

# Runs one campaign model for a problem into summary/raw paths.
function Invoke-CampaignModel {
    param(
        [string]$Problem,
        [string]$Exe,
        [string]$Model,
        [int]$N,
        [int]$W,
        [string]$SummaryPath,
        [string]$RawPath
    )
    if ($Problem -eq "boids") {
        & $Exe --model $Model --boids $N --steps $Steps --workers $W `
            --smt-factor $SmtFactor --runs $Runs --warmup $Warmup `
            --seed $Seed --output $SummaryPath --raw-output $RawPath
        if ($LASTEXITCODE -ne 0) { throw "boids_benchmark ($Model) failed" }
    }
    elseif ($Problem -eq "liquids") {
        & $Exe --model $Model --particles $N --steps $Steps --workers $W `
            --quantum $Quantum --chunk 64 --smt-factor $SmtFactor `
            --runs $Runs --warmup $Warmup --seed $Seed `
            --output $SummaryPath --raw-output $RawPath
        if ($LASTEXITCODE -ne 0) { throw "liquids_benchmark ($Model) failed" }
    }
    elseif ($Problem -eq "ray") {
        $res = Get-RayResolution $N
        & $Exe --model $Model --runs $Runs --warmup $Warmup --workers $W `
            --width $res.Width --height $res.Height --seed $Seed `
            --smt-factor $SmtFactor --quantum $Quantum `
            --output $SummaryPath --raw-output $RawPath
        if ($LASTEXITCODE -ne 0) { throw "raytracing_benchmark ($Model) failed" }
    }
    elseif ($Problem -eq "nbody") {
        $legacy = ($SummaryPath -replace '_summary\.csv$', '_legacy.csv')
        & $Exe -headless --model $Model -n $N --steps $Steps `
            --runs $Runs --warmup $Warmup --workers $W `
            --smt-factor $SmtFactor --seed $Seed `
            --output $SummaryPath --raw-output $RawPath -o $legacy
        if ($LASTEXITCODE -ne 0) { throw "n_body ($Model) failed" }
    }
    else {
        throw "Unknown problem: $Problem"
    }
}

function Invoke-UnifiedBuild {
    Refresh-Path
    $cmake = Find-CMake
    if (-not $cmake) {
        throw "Missing cmake. Run setup_windows.ps1 first."
    }

    $generatorArgs = @()
    if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
        if (Get-Command g++ -ErrorAction SilentlyContinue) {
            $generatorArgs = @("-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release")
        } else {
            throw "No C++ compiler (cl/g++) on PATH."
        }
    }

    $nbodyEnable = if ($ProblemSet.ContainsKey("nbody")) { "ON" } else { "OFF" }
    $cmakeArgs = @(
        "-S", $P1Root,
        "-B", $BuildDir,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBOIDS_BUILD_VISUAL=OFF",
        "-DBOIDS_BUILD_TESTS=OFF",
        "-DLIQUIDS_BUILD_TESTS=OFF",
        "-DBUILD_TESTING=OFF",
        "-DNBODY_ENABLE=$nbodyEnable"
    ) + $generatorArgs

    if (-not [string]::IsNullOrWhiteSpace($RaylibRoot)) {
        $cmakeArgs += "-DNBODY_RAYLIB_ROOT=$RaylibRoot"
    }

    Write-Info "Configuring unified CMake ($BuildDir)"
    & $cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

    Write-Info "Building Release"
    & $cmake --build $BuildDir --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }
}

# --- main --------------------------------------------------------------------

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$SummaryAll = Join-Path $OutputDir "campaign_summary_all.csv"
$RawAll = Join-Path $OutputDir "campaign_raw_all.csv"
$Manifest = Join-Path $OutputDir "manifest.csv"
"problem,workload_n,workers,summary_csv,raw_csv" | Set-Content $Manifest

if (-not $SkipBuild) {
    Invoke-UnifiedBuild
}

$bins = @{}
if ($ProblemSet.ContainsKey("boids")) {
    $bins["boids"] = Find-Exe "boids_benchmark"
    if (-not $bins["boids"]) { throw "boids_benchmark not found under $BuildDir" }
}
if ($ProblemSet.ContainsKey("liquids")) {
    $bins["liquids"] = Find-Exe "liquids_benchmark"
    if (-not $bins["liquids"]) { throw "liquids_benchmark not found under $BuildDir" }
}
if ($ProblemSet.ContainsKey("ray")) {
    $bins["ray"] = Find-Exe "raytracing_benchmark"
    if (-not $bins["ray"]) { throw "raytracing_benchmark not found under $BuildDir" }
}
if ($ProblemSet.ContainsKey("nbody")) {
    $bins["nbody"] = Find-Exe "n_body"
    if (-not $bins["nbody"]) { throw "n_body not found under $BuildDir (Raylib required)" }
}

$summaryHeaderWritten = $false
$rawHeaderWritten = $false
# Parallel models re-run per worker; sequential is cached once per workload.
$ParallelModels = @("fine", "coarse", "smt", "cmp")

foreach ($problem in @("boids", "liquids", "ray", "nbody")) {
    if (-not $ProblemSet.ContainsKey($problem)) { continue }
    $problemDir = Join-Path $OutputDir $problem
    New-Item -ItemType Directory -Force -Path $problemDir | Out-Null
    $exe = $bins[$problem]

    foreach ($N in $Workloads) {
        # Sequential baseline: one measurement per workload (workers=1).
        $seqPrefix = Join-Path $problemDir ("n_${N}_sequential")
        $seqSummary = "${seqPrefix}_summary.csv"
        $seqRaw = "${seqPrefix}_raw.csv"
        Write-Info "$problem workload=$N model=sequential (once, workers=1) runs=$Runs warmup=$Warmup"
        Invoke-CampaignModel -Problem $problem -Exe $exe -Model "sequential" `
            -N $N -W 1 -SummaryPath $seqSummary -RawPath $seqRaw

        foreach ($W in $Workers) {
            $prefix = Join-Path $problemDir ("n_${N}_w_${W}")
            $summary = "${prefix}_summary.csv"
            $raw = "${prefix}_raw.csv"
            # Cell CSV = replicated sequential rows + parallel models for this W.
            $partSummaries = @($seqSummary)
            $partRaws = @()
            if (Test-Path -LiteralPath $seqRaw -PathType Leaf) {
                $partRaws += $seqRaw
            }

            foreach ($model in $ParallelModels) {
                Write-Info "$problem workload=$N workers=$W model=$model runs=$Runs warmup=$Warmup"
                $ms = "${prefix}_${model}_summary.csv"
                $mr = "${prefix}_${model}_raw.csv"
                Invoke-CampaignModel -Problem $problem -Exe $exe -Model $model `
                    -N $N -W $W -SummaryPath $ms -RawPath $mr
                $partSummaries += $ms
                # Ray may not write a leaf raw at $mr; only merge existing files.
                if (Test-Path -LiteralPath $mr -PathType Leaf) {
                    $partRaws += $mr
                }
            }

            Merge-CsvFiles -DestPath $summary -SourcePaths $partSummaries
            if ($partRaws.Count -gt 0) {
                Merge-CsvFiles -DestPath $raw -SourcePaths $partRaws
            }

            Merge-CsvWithProblem -SourcePath $summary -Problem $problem `
                -DestPath $SummaryAll -HeaderWritten ([ref]$summaryHeaderWritten)
            if (Test-Path -LiteralPath $raw -PathType Leaf) {
                Merge-CsvWithProblem -SourcePath $raw -Problem $problem `
                    -DestPath $RawAll -HeaderWritten ([ref]$rawHeaderWritten)
            }
            "$problem,$N,$W,$summary,$raw" | Add-Content $Manifest
        }
    }
}

Write-Host "Campaign completed." -ForegroundColor Green
Write-Host "  Output:  $OutputDir"
Write-Host "  Summary: $SummaryAll"
Write-Host "  Raw:     $RawAll"
