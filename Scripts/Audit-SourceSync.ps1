# Read-only audit of UE's serialized FAssetImportInfo JSON and original sources.
# Does not launch Unreal or write to content; safe while gameplay reads assets.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$records = [System.Collections.Generic.List[object]]::new()
$groups = @(
    @{ Folder = 'Exports'; Filter = '*.fbx' },
    @{ Folder = 'Textures'; Filter = '*.png' },
    @{ Folder = 'Audio'; Filter = '*.wav' }
)
foreach ($group in $groups) {
    $sourceDirectory = Join-Path $projectRoot ('ArtSource/' + $group.Folder)
    foreach ($sourceFile in Get-ChildItem -LiteralPath $sourceDirectory -Filter $group.Filter | Sort-Object Name) {
        if ($group.Folder -eq 'Audio') { $assetFolder = 'Audio' }
        elseif ($group.Folder -eq 'Textures') { $assetFolder = 'Textures' }
        elseif ($sourceFile.BaseName.StartsWith('A_')) { $assetFolder = 'Animations' }
        elseif ($sourceFile.BaseName.StartsWith('SK_') -or $sourceFile.BaseName.StartsWith('SM_Infected')) { $assetFolder = 'Characters' }
        else { $assetFolder = 'Art/Environment' }
        $assetRelative = 'Content/ONE/' + $assetFolder + '/' + $sourceFile.BaseName + '.uasset'
        $assetFile = Join-Path $projectRoot $assetRelative
        if (-not (Test-Path -LiteralPath $assetFile -PathType Leaf)) { throw "Missing imported asset: $assetFile" }
        $assetText = [System.Text.Encoding]::UTF8.GetString([System.IO.File]::ReadAllBytes($assetFile))
        $jsonMatch = [regex]::Match($assetText, '\[\s*\{\s*"RelativeFilename"[^\x00]+?\}\s*\]')
        if (-not $jsonMatch.Success) { throw "Missing serialized import metadata: $assetFile" }
        $sourceMetadata = @($jsonMatch.Value | ConvertFrom-Json)
        $recorded = $sourceMetadata | Where-Object { [System.IO.Path]::GetFileName($_.RelativeFilename) -eq $sourceFile.Name } | Select-Object -First 1
        if (-not $recorded) { throw "Source filename mismatch in: $assetFile" }
        $sourceMd5 = (Get-FileHash -LiteralPath $sourceFile.FullName -Algorithm MD5).Hash.ToLowerInvariant()
        $importedMd5 = ([string]$recorded.FileMD5).ToLowerInvariant()
        if ($sourceMd5 -ne $importedMd5) { throw "Stale imported asset $assetRelative : source=$sourceMd5 imported=$importedMd5" }
        $records.Add([ordered]@{
            source = 'ArtSource/' + $group.Folder + '/' + $sourceFile.Name
            asset = '/Game/ONE/' + $assetFolder + '/' + $sourceFile.BaseName
            md5 = $sourceMd5
            matching_import_hash = $true
        })
    }
}
$report = [ordered]@{
    verified_utc = [DateTime]::UtcNow.ToString('o')
    method = 'Compared original source MD5 with FileMD5 in serialized Unreal FAssetImportInfo JSON; read-only filesystem audit'
    count = $records.Count
    all_source_hashes_match = $true
    sources = $records
}
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $projectRoot 'Evidence/final_source_sync.json')
Write-Output "ONE FINAL SOURCE HASHES ALL MATCH: $($records.Count) original sources"
