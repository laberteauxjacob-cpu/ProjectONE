# Read-only audit of UE's serialized FAssetImportInfo and authored sources.
# Does not launch Unreal or write to content; safe while gameplay reads assets.
param([string]$OutputRelativePath = 'Evidence/Candidate02/source_sync.json', [switch]$Candidate03)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ($Candidate03 -and -not $PSBoundParameters.ContainsKey('OutputRelativePath')) {
    $OutputRelativePath = 'Evidence/Candidate03/source_sync.json'
}
$records = [System.Collections.Generic.List[object]]::new()
$sources = [System.Collections.Generic.List[object]]::new()
# The accepted report supplies its explicit 39-path inventory; old hashes remain
# historical evidence and are not used as today's expected source hashes.
$baseline = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Evidence/final_source_sync.json') | ConvertFrom-Json
if ($baseline.sources.Count -ne 39) { throw 'Expected the accepted 39-source Candidate01 inventory' }
foreach ($entry in $baseline.sources) { $sources.Add(@{source=$entry.source; asset=$entry.asset; candidate='01'}) }
$candidate = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'ArtSource/Weapons/Candidate02/inventory.json') | ConvertFrom-Json
foreach ($name in $candidate.static_meshes) { $sources.Add(@{source="ArtSource/Exports/$name.fbx"; asset="/Game/ONE/Art/Weapons/$name"; candidate='02'}) }
$clips = @($candidate.animations.PSObject.Properties.Name) + @($candidate.infected_reaction.name)
foreach ($name in $clips) { $sources.Add(@{source="ArtSource/Exports/$name.fbx"; asset="/Game/ONE/Animations/$name"; candidate='02'}) }
foreach ($entry in $candidate.audio.PSObject.Properties.Value) { $sources.Add(@{source=$entry.source; asset=$entry.asset; candidate='02'}) }
if ($candidate.static_meshes.Count -ne 5 -or $clips.Count -ne 9 -or $candidate.audio.PSObject.Properties.Name.Count -ne 25 -or $sources.Count -ne 78) {
    throw 'Expected 39 accepted sources plus 14 new FBXs and 25 new WAVs'
}
$candidate03Count = 0
if ($Candidate03) {
    # Explicit Stage B inventory. Extend deliberately when Stage C/D is accepted.
    $locomotion = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'ArtSource/Characters/Candidate03/inventory.json') | ConvertFrom-Json
    $expectedClips = @('A_Response_C03_Turn_L', 'A_Response_C03_Turn_R')
    foreach ($gait in @('Walk', 'Run')) {
        foreach ($direction in @('F', 'FR', 'R', 'BR', 'B', 'BL', 'L', 'FL')) {
            $expectedClips += "A_Response_C03_${gait}_${direction}"
        }
    }
    $actualClips = @($locomotion.clips.PSObject.Properties.Name)
    if ($actualClips.Count -ne 18 -or @(Compare-Object ($expectedClips | Sort-Object) ($actualClips | Sort-Object)).Count -ne 0) {
        throw 'Expected the explicit eighteen Candidate03 Stage B animations'
    }
    foreach ($name in ($expectedClips | Sort-Object)) {
        $sources.Add(@{source="ArtSource/Exports/Candidate03/$name.fbx"; asset="/Game/ONE/Animations/$name"; candidate='03'})
    }
    $candidate03Count = 18
}
if (@($sources.source | Sort-Object -Unique).Count -ne $sources.Count) { throw 'Duplicate source in inventory' }
foreach ($entry in $sources) {
        $sourceFile = Get-Item -LiteralPath (Join-Path $projectRoot $entry.source)
        $assetRelative = 'Content/' + $entry.asset.Substring('/Game/'.Length) + '.uasset'
        $assetFile = Join-Path $projectRoot $assetRelative
        if (-not (Test-Path -LiteralPath $assetFile -PathType Leaf)) { throw "Missing imported asset: $assetFile" }
        $assetText = [System.Text.Encoding]::UTF8.GetString([System.IO.File]::ReadAllBytes($assetFile))
        $jsonMatch = [regex]::Match($assetText, '\[\s*\{\s*"RelativeFilename"[^\x00]+?\}\s*\]')
        if (-not $jsonMatch.Success) { throw "Missing serialized import metadata: $assetFile" }
        $sourceMetadata = @($jsonMatch.Value | ConvertFrom-Json)
        $recorded = $sourceMetadata | Where-Object { [System.IO.Path]::GetFileName($_.RelativeFilename) -eq $sourceFile.Name } | Select-Object -First 1
        if (-not $recorded) { throw "Source filename mismatch in: $assetFile" }
        if ($entry.candidate -eq '03') {
            if ($sourceMetadata.Count -ne 1 -or [IO.Path]::IsPathRooted([string]$recorded.RelativeFilename)) { throw "Unexpected Candidate03 import path: $assetRelative" }
            $resolvedImport = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $assetFile) $recorded.RelativeFilename))
            if ($resolvedImport -ne $sourceFile.FullName) { throw "Candidate03 import path resolves outside its inventory: $assetRelative" }
        }
        $sourceMd5 = (Get-FileHash -LiteralPath $sourceFile.FullName -Algorithm MD5).Hash.ToLowerInvariant()
        $importedMd5 = ([string]$recorded.FileMD5).ToLowerInvariant()
        if ($sourceMd5 -ne $importedMd5) { throw "Stale imported asset $assetRelative : source=$sourceMd5 imported=$importedMd5" }
        $records.Add([ordered]@{
            source = $entry.source
            asset = $entry.asset
            introduced_in_candidate = $entry.candidate
            md5 = $sourceMd5
            matching_import_hash = $true
        })
}
$report = [ordered]@{
    verified_utc = [DateTime]::UtcNow.ToString('o')
    method = 'Compared original source MD5 with FileMD5 in serialized Unreal FAssetImportInfo JSON; read-only filesystem audit'
    count = $records.Count
    accepted_candidate01_sources = 39
    new_candidate02_fbx = 14
    new_candidate02_wav = 25
    all_source_hashes_match = $true
    sources = $records
}
if ($Candidate03) { $report['new_candidate03_stage_b_fbx'] = $candidate03Count }
$reportPath = Join-Path $projectRoot $OutputRelativePath
if (-not [IO.Path]::GetFullPath($reportPath).StartsWith([IO.Path]::GetFullPath($projectRoot) + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Audit output must remain inside the project' }
if ([System.IO.Path]::GetFullPath($reportPath) -eq [System.IO.Path]::GetFullPath((Join-Path $projectRoot 'Evidence/final_source_sync.json'))) { throw 'Candidate01 historical evidence must remain unchanged' }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $reportPath) | Out-Null
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath
if ($Candidate03) {
    Write-Output "ONE CANDIDATE03 SOURCE HASHES ALL MATCH: $($records.Count) sources (78 accepted + 18 Stage B FBX)"
} else {
    Write-Output "ONE CANDIDATE02 SOURCE HASHES ALL MATCH: $($records.Count) sources (39 accepted + 14 FBX + 25 WAV)"
}
