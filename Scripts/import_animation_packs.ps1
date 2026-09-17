<#
.SYNOPSIS
    Copies the owned animation packs into Content/.

.DESCRIPTION
    The packs are excluded from source control (see .gitignore), so a fresh clone
    needs them copied in before BP_Operative can resolve its skeletal mesh.

    Folder names must be preserved: the packs' internal references are absolute
    /Game/OpenWorldAnimset/... and /Game/FightingAnimsetPro/... paths, which break
    if the assets are moved elsewhere.

.EXAMPLE
    ./Scripts/import_animation_packs.ps1
    ./Scripts/import_animation_packs.ps1 -SourceProject "D:\Projects\SomeOtherProject"
#>
param(
    [string]$SourceProject = "$env:USERPROFILE\Documents\Unreal Projects\DynamicLocomotion"
)

$ErrorActionPreference = "Stop"

$dest = Join-Path (Split-Path $PSScriptRoot -Parent) "Content"
$packs = @("OpenWorldAnimset", "FightingAnimsetPro")

foreach ($pack in $packs) {
    $src = Join-Path $SourceProject "Content\$pack"
    if (-not (Test-Path $src)) {
        Write-Error "Pack not found: $src"
    }

    $target = Join-Path $dest $pack
    if (Test-Path $target) {
        Write-Host "already present, skipping: $pack"
        continue
    }

    Write-Host "copying $pack ..."
    Copy-Item -Path $src -Destination $target -Recurse
}

Write-Host "done."
