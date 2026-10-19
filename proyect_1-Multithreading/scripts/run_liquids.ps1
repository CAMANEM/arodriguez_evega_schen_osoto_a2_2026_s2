param(
    [ValidateSet("build", "test", "benchmark", "campaign", "all")]
    [string]$Mode = "benchmark"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$LiquidsSource = Join-Path $RepoRoot "proyect_1-Multithreading/liquids"
$BuildDir = Join-Path $RepoRoot "build/liquids"

function Invoke-Build {
    cmake -S $LiquidsSource -B $BuildDir -DCMAKE_BUILD_TYPE=Release
    cmake --build $BuildDir --config Release
}

function Get-BenchmarkPath {
    $paths = @(
        (Join-Path $BuildDir "liquids_benchmark"),
        (Join-Path $BuildDir "Release/liquids_benchmark.exe")
    )
    foreach ($path in $paths) {
        if (Test-Path $path) { return $path }
    }
    throw "liquids_benchmark was not found."
}

switch ($Mode) {
    "build" {
        Invoke-Build
    }
    "test" {
        Invoke-Build
        ctest --test-dir $BuildDir --output-on-failure
    }
    "benchmark" {
        Invoke-Build
        ctest --test-dir $BuildDir --output-on-failure
        $Benchmark = Get-BenchmarkPath
        $Output = Join-Path $RepoRoot "proyect_1-Multithreading/liquids/data/benchmark.csv"
        & $Benchmark --model all --particles 1000 --steps 10 `
            --threads 4 --quantum 8 --chunk 64 --smt-factor 2 --runs 5 `
            --output $Output
    }
    "campaign" {
        $Campaign = Join-Path $PSScriptRoot "campaign_liquids.ps1"
        & $Campaign
    }
    "all" {
        Invoke-Build
        ctest --test-dir $BuildDir --output-on-failure
        $Benchmark = Get-BenchmarkPath
        $Output = Join-Path $RepoRoot "proyect_1-Multithreading/liquids/data/benchmark.csv"
        & $Benchmark --model all --particles 1000 --steps 10 `
            --threads 4 --quantum 8 --chunk 64 --smt-factor 2 --runs 5 `
            --output $Output
    }
}
