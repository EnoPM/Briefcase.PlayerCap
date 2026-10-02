[CmdletBinding()]
param([Parameter(Mandatory)][string]$Archive)

$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$version = (Get-Content -LiteralPath (Join-Path $repository 'VERSION') -Raw).Trim()
$expected = [IO.Path]::GetFullPath((Join-Path $repository "dist\Briefcase.PlayerCap-windows-x64-$version.zip"))
$archivePath = [IO.Path]::GetFullPath($Archive)
if ($archivePath -ine $expected) { throw 'Unexpected PlayerCap archive path.' }

Add-Type -AssemblyName System.IO.Compression.FileSystem
$legacyConfig = 'ue4ss/Mods/BriefcasePlayerCap/Data/config.json'
$zip = [IO.Compression.ZipFile]::Open($archivePath, [IO.Compression.ZipArchiveMode]::Update)
try {
    $entry = $zip.GetEntry($legacyConfig)
    if (-not $entry) { throw 'Legacy config is missing from the package being corrected.' }
    $entry.Delete()
} finally {
    $zip.Dispose()
}

$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    if ($zip.GetEntry($legacyConfig)) { throw 'Legacy config remains in PlayerCap archive.' }
    foreach ($required in @('ue4ss/Mods/BriefcasePlayerCap/dlls/main.dll',
                            'ue4ss/Mods/BriefcasePlayerCap/BriefcasePreEntry.dll')) {
        if (-not $zip.GetEntry($required)) { throw "PlayerCap archive is missing $required" }
    }
} finally {
    $zip.Dispose()
}

Write-Output 'PlayerCap archive contains no obsolete Solo/Duo configuration.'
