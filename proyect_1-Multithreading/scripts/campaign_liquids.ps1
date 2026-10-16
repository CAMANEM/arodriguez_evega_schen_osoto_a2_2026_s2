param(
    [int]$Runs = 200,
    [int]$Steps = 10,
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"
if ($Runs -le 0 -or $Steps -le 0) {
    throw "Runs and Steps must be positive."
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$LiquidsSource = Join-Path $RepoRoot "proyect_1-Multithreading/liquids"
$BuildDir = Join-Path $RepoRoot "build/liquids"
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $RepoRoot "proyect_1-Multithreading/liquids/data/campaign"
}

cmake -S $LiquidsSource -B $BuildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $BuildDir --config Release
ctest --test-dir $BuildDir --output-on-failure

$Benchmark = Join-Path $BuildDir "liquids_benchmark"
if (-not (Test-Path $Benchmark)) {
    $Benchmark = Join-Path $BuildDir "Release/liquids_benchmark.exe"
}
if (-not (Test-Path $Benchmark)) {
    throw "liquids_benchmark was not found."
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
"particles,threads,summary_csv,raw_csv" |
    Set-Content (Join-Path $OutputDir "manifest.csv")

foreach ($Particles in @(100, 1000, 10000)) {
    foreach ($Threads in @(2, 4, 8)) {
        $Prefix = Join-Path $OutputDir "particles_${Particles}_threads_${Threads}"
        $Summary = "${Prefix}_summary.csv"
        $Raw = "${Prefix}_raw.csv"
        Write-Host "==> particles=$Particles threads=$Threads runs=$Runs steps=$Steps"
        & $Benchmark --model all --particles $Particles --steps $Steps `
            --threads $Threads --quantum 8 --chunk 64 --smt-factor 2 `
            --runs $Runs --output $Summary --raw-output $Raw
        "$Particles,$Threads,$Summary,$Raw" |
            Add-Content (Join-Path $OutputDir "manifest.csv")
    }
}

Write-Host "Campaign completed. Results: $OutputDir"
