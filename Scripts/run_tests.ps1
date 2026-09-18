<#
.SYNOPSIS
    Runs every automated test headless and reports a combined result.
.DESCRIPTION
    One editor launch, two kinds of test, both Unreal's own:
      ExecutiveOps.*             world-free checks on a subsystem or component
      Project.Functional Tests.* AFunctionalTest actors placed in the maps -
                                 the flight sequence in L_FlightTest, the
                                 ground route and the mission in L_MissionTest
    Every test in a map runs in that map's one world, in turn.
    Exits non-zero if any test fails, so this is usable as a gate.
.PARAMETER Filter
    An automation test filter, as the Session Frontend or `Automation RunTests`
    takes it. Defaults to both groups; e.g. "Project.Functional Tests.Maps.L_FlightTest"
    runs one map.
.EXAMPLE
    ./Scripts/run_tests.ps1
    ./Scripts/run_tests.ps1 -Filter "ExecutiveOps"
#>
param(
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string]$Filter = "ExecutiveOps+Project.Functional Tests",
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

$project = Join-Path (Split-Path $PSScriptRoot -Parent) "ExecutiveOps.uproject"
$editor = Join-Path $Engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$log = Join-Path (Split-Path $PSScriptRoot -Parent) "Saved\Logs\Tests.log"

if (-not (Test-Path $editor)) { Write-Error "Editor not found: $editor" }
if (Test-Path $log) { Remove-Item $log -Force -ErrorAction SilentlyContinue }

Write-Host "running '$Filter' ..."

$args = @($project,
          "-ExecCmds=`"Automation RunTests $Filter;Quit`"",
          "-TestExit=`"Automation Test Queue Empty`"",
          "-unattended", "-nullrhi", "-nosplash", "-nopause",
          "-abslog=`"$log`"")

& $editor @args 2>&1 | Out-Null

# The log is authoritative, not the exit code: a graceful engine shutdown does
# not reliably propagate a status, so trusting the exit code alone reports
# failing runs as passing.
if (-not (Test-Path $log)) {
    Write-Host "NO RESULT - the editor wrote no log" -ForegroundColor Red
    exit 1
}

if ($Verbose) {
    Select-String -Path $log -Pattern "\[SelfTest\]" | ForEach-Object {
        Write-Host ("  " + ($_.Line -replace '^.*LogExecutiveOpsDev: ', '' -replace '^(Display|Error): ', ''))
    }
}

$results = Select-String -Path $log -Pattern "Test Completed\. Result=\{(\w+)\} Name=\{([^}]*)\} Path=\{([^}]*)\}" |
    ForEach-Object {
        [pscustomobject]@{
            Result = $_.Matches[0].Groups[1].Value
            Path   = $_.Matches[0].Groups[3].Value
        }
    }

if (-not $results) {
    Write-Host "NO RESULT - no test completed; see $log" -ForegroundColor Red
    exit 1
}

$failed = 0
foreach ($r in $results) {
    if ($r.Result -eq "Success") {
        Write-Host ("  {0,-8} {1}" -f "passed", $r.Path) -ForegroundColor Green
    } else {
        Write-Host ("  {0,-8} {1}" -f $r.Result.ToUpper(), $r.Path) -ForegroundColor Red
        $failed++
    }
}

if ($failed -gt 0) {
    # The framework's own reasons: failed assertions, and any warning or error
    # the test did not declare it expected.
    Select-String -Path $log -Pattern "LogAutomationController: Error: " |
        Where-Object { $_.Line -notmatch "Test Completed" } |
        ForEach-Object { Write-Host ("    " + ($_.Line -replace '^.*LogAutomationController: Error: ', '')) -ForegroundColor Red }
    Write-Host "$failed of $($results.Count) test(s) failed." -ForegroundColor Red
    exit 1
}

Write-Host "all $($results.Count) tests passed." -ForegroundColor Green
exit 0
