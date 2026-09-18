<#
.SYNOPSIS
    Copies the licensed asset packs into Content/.

.DESCRIPTION
    Every pack listed below is excluded from source control (see .gitignore).
    They are store-licensed content each developer obtains through their own Epic
    account, and together they run to several gigabytes - well past GitHub's free
    Git LFS quota.

    Folder names are preserved exactly. The packs' internal references are
    absolute /Game/<PackFolder>/... paths and break if the assets are moved.

    Sources, in the order the script looks:

      1. The Epic Games Launcher vault cache, where a Fab download lands:
         C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\<Pack>\data\Content\...
      2. A sibling Unreal project, for packs distributed as complete projects
         (Lyra) or already imported somewhere else (Niagara Examples).

    Nothing here is required to build or run the project. A clone without the
    packs compiles, launches and plays - it is simply silent, because every
    feedback preset resolves to a missing soft reference. See Docs/M8-Feedback.md.

.PARAMETER VaultCache
    Override the launcher's vault cache location.

.PARAMETER ProjectsRoot
    Where sibling Unreal projects live.

.PARAMETER Include
    Only import packs whose name matches one of these (wildcards allowed).

.PARAMETER List
    Report what is present, missing and already imported, and change nothing.

.PARAMETER Force
    Re-copy packs that are already in Content/.

.EXAMPLE
    ./Scripts/import_fab_assets.ps1 -List
    ./Scripts/import_fab_assets.ps1
    ./Scripts/import_fab_assets.ps1 -Include NiagaraExamples,Lyra*
#>
param(
    [string]$VaultCache = "C:\ProgramData\Epic\EpicGamesLauncher\VaultCache",
    [string]$ProjectsRoot = "$env:USERPROFILE\Documents\Unreal Projects",
    [string[]]$Include,
    [switch]$List,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$root = Split-Path $PSScriptRoot -Parent
$contentDir = Join-Path $root "Content"

# Folder = the name the pack must keep under Content/.
# VaultDir = the vault cache directory, whose data/Content/<Folder> we copy.
# Project/ProjectPath = fallback source inside a sibling .uproject's Content.
# Purpose = which M8 verbs it serves, so this table doubles as the reason each
#           pack is on disk at all.
$packs = @(
    @{ Folder = "OpenWorldAnimset";      VaultDir = "OpenWorl0de4751fa43eV1";  Project = "DynamicLocomotion"; Purpose = "Locomotion, vault, climb animation" }
    @{ Folder = "FightingAnimsetPro";    VaultDir = "FightingAnimsetPro";      Project = "DynamicLocomotion"; Purpose = "Takedown and melee animation" }
    @{ Folder = "DynamicLocomotion";     VaultDir = "DynamicL7332708e784bV1";  Project = "DynamicLocomotion"; Purpose = "Locomotion graph: starts, stops, landings, sync-marked walk/jog/run" }
    @{ Folder = "BigNiagaraBundle";      VaultDir = "BigNiagafb970ed92a8eV2";  Purpose = "Thruster jets, dust, sparks, holograms" }
    @{ Folder = "Chameleon";             VaultDir = "Chameleo527966e04eb8V15"; Purpose = "Post-process grade, alarm material, bullet hole texture" }
    @{ Folder = "ChameleonLayers";       VaultDir = "Chameleo527966e04eb8V15"; Purpose = "Chameleon layer stack" }
    @{ Folder = "HumanVocalizations";    VaultDir = "HumanVocalizations";      Purpose = "Takedown, guard pain and death, player effort" }
    @{ Folder = "Essential_Foosteps_SK"; VaultDir = "Essentiaac2532a6e5e4V1";  Purpose = "Footsteps, landings, stops by surface" }
    @{ Folder = "FreeWeaponSounds";      VaultDir = "FreeWeapaf55c7129267V1";  Purpose = "Pistol shot, mechanism, tails" }
    @{ Folder = "SciFiUISFX";            VaultDir = "SCIFIUISa00ced1f1d38V1";  Purpose = "UI and detection state cues" }
    @{ Folder = "NW_MuzzleFX";           VaultDir = "NEONWEXF92be5882b5a1V1";  Purpose = "Muzzle flashes, shot bursts, shell and smoke" }
    @{ Folder = "Vefects";               VaultDir = "EasyImpa76514c37f081V1";  Purpose = "Comic impact frames for takedown" }
    @{ Folder = "NiagaraExamples";       VaultDir = "NiagaraExamplesPack";     Project = "HoldingProject"; Purpose = "Bullet impacts by material, sparks, tracers" }
    @{ Folder = "Audio";                 Project = "LyraStarterGame";          Purpose = "Bullet whizz-bys, impacts, weapon and foley MetaSounds" }
)

function Resolve-PackSource($pack) {
    if ($pack.VaultDir) {
        $candidate = Join-Path $VaultCache "$($pack.VaultDir)\data\Content\$($pack.Folder)"
        if (Test-Path $candidate) { return $candidate }
    }

    if ($pack.Project) {
        $relative = if ($pack.ProjectPath) { $pack.ProjectPath } else { $pack.Folder }
        $candidate = Join-Path $ProjectsRoot "$($pack.Project)\Content\$relative"
        if (Test-Path $candidate) { return $candidate }
    }

    return $null
}

$selected = $packs | Where-Object {
    if (-not $Include) { return $true }
    $folder = $_.Folder
    ($Include | Where-Object { $folder -like $_ }).Count -gt 0
}

if ($selected.Count -eq 0) {
    Write-Warning "No packs matched -Include. Known packs: $(($packs | ForEach-Object { $_.Folder }) -join ', ')"
    return
}

$imported = 0
$skipped = 0
$missing = @()

foreach ($pack in $selected) {
    $target = Join-Path $contentDir $pack.Folder
    $source = Resolve-PackSource $pack

    if ($List) {
        $state = if (Test-Path $target) { "imported" }
                 elseif ($source)       { "available" }
                 else                   { "MISSING" }

        "{0,-22} {1,-10} {2}" -f $pack.Folder, $state, $pack.Purpose | Write-Host
        continue
    }

    if ((Test-Path $target) -and -not $Force) {
        Write-Host "already present, skipping: $($pack.Folder)"
        $skipped++
        continue
    }

    if (-not $source) {
        Write-Warning "not found, skipping: $($pack.Folder) - download it from Fab first ($($pack.Purpose))"
        $missing += $pack.Folder
        continue
    }

    if ((Test-Path $target) -and $Force) {
        Write-Host "replacing $($pack.Folder) ..."
        Remove-Item -Path $target -Recurse -Force
    }

    Write-Host "copying $($pack.Folder) ..."
    Copy-Item -Path $source -Destination $target -Recurse
    $imported++
}

if ($List) { return }

Write-Host ""
Write-Host "imported $imported, skipped $skipped, missing $($missing.Count)"

if ($missing.Count -gt 0) {
    Write-Host "missing: $($missing -join ', ')"
    Write-Host "The project still builds and runs without these; the affected feedback events stay silent."
}

Write-Host "Next: rebuild the feedback presets with"
Write-Host '  UnrealEditor-Cmd.exe ExecutiveOps.uproject -run=pythonscript -script="Scripts/m8_build_feedback_presets.py"'
