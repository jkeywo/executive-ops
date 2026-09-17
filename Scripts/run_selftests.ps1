<#
.SYNOPSIS
    Runs every self-test suite and reports a combined result.

.DESCRIPTION
    Two suites, because the flight sequence and the ground route live in
    different maps:

      L_FlightTest   boot, flight model, navigation, the approach, deployment
      L_MissionTest  traversal verbs and slide

    Exits non-zero if either suite fails, so this is usable as a gate.

.EXAMPLE
    ./Scripts/run_selftests.ps1
#>
param(
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

$project = Join-Path (Split-Path $PSScriptRoot -Parent) "ExecutiveOps.uproject"
$editor = Join-Path $Engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$log = Join-Path (Split-Path $PSScriptRoot -Parent) "Saved\Logs\ExecutiveOps.log"

if (-not (Test-Path $editor)) { Write-Error "Editor not found: $editor" }

$suites = @(
    @{ Name = "flight"; Map = "/Game/Maps/L_FlightTest";  Extra = @() },
    @{ Name = "ground"; Map = "/Game/Maps/L_MissionTest"; Extra = @("-EOGroundTest") }
)

$failed = 0

foreach ($suite in $suites) {
    Write-Host "running $($suite.Name) suite ..."

    $args = @($project, $suite.Map, "-game", "-nullrhi", "-unattended",
              "-nosplash", "-nopause", "-EOSelfTest", "-EOSelfTestExit") + $suite.Extra

    if (Test-Path $log) { Remove-Item $log -Force }

    & $editor @args 2>&1 | Out-Null
    $code = $LASTEXITCODE

    # The log is authoritative, not the exit code: a graceful engine shutdown
    # does not reliably propagate RequestExitWithStatus, so trusting the exit
    # code alone silently reports failing suites as passing.
    $verdict = $null
    $lines = @()
    if (Test-Path $log) {
        $lines = Select-String -Path $log -Pattern "\[SelfTest\]" | ForEach-Object {
            $_.Line -replace '^.*LogExecutiveOps: ', '' -replace '^(Display|Error): ', ''
        }
        if ($Verbose) { $lines | ForEach-Object { Write-Host "  $_" } }
        else { $lines | Where-Object { $_ -match "FAIL|===" } | ForEach-Object { Write-Host "  $_" } }

        $verdict = $lines | Where-Object { $_ -match "=== (PASSED|FAILED)" } | Select-Object -Last 1
    }

    $ok = $true
    if ($null -eq $verdict) {
        Write-Host "  $($suite.Name): NO RESULT - the suite did not run to completion" -ForegroundColor Red
        $ok = $false
    } elseif ($verdict -match "FAILED") {
        $ok = $false
    } elseif ($code -ne 0) {
        Write-Host "  $($suite.Name): suite reported PASSED but the process exited $code" -ForegroundColor Yellow
    }

    if ($ok) {
        Write-Host "  $($suite.Name): passed" -ForegroundColor Green
    } else {
        Write-Host "  $($suite.Name): FAILED" -ForegroundColor Red
        $failed++
    }
}

if ($failed -gt 0) {
    Write-Host "$failed suite(s) failed." -ForegroundColor Red
    exit 1
}

Write-Host "all suites passed." -ForegroundColor Green
exit 0
