param(
    [string]$EngineRoot = $env:UE_ROOT,
    [ValidateSet('Candidate03','Candidate04','Candidate05','Candidate06','Candidate07')][string]$Candidate = 'Candidate07',
    [switch]$PlanOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
. "$PSScriptRoot\Engine.ps1"
$engineRoot = Resolve-ONEEngine -EngineRoot $EngineRoot
$maps = if ($Candidate -in @('Candidate06','Candidate07')) { '/Game/ONE/Maps/Containment+/Game/ONE/Maps/Portability06' } else { '/Game/ONE/Maps/Containment' }
$destination = [IO.Path]::GetFullPath((Join-Path $projectRoot "Packaged\$Candidate"))
$contract = 'Package creation is not release verification. Fresh public source/LFS, engine automation, exact runtime hashes/privacy, requested packaged checks, archive inspection and public downloads are separate gates.'
if ($PlanOnly) {
    [ordered]@{ candidate=$Candidate; maps=$maps; package_directory=$destination; existing_output=(Test-Path -LiteralPath $destination); overwrite_allowed=$false; launches_engine=$false; release_verified=$false; verification_contract=$contract } | ConvertTo-Json
    return
}
if (Test-Path -LiteralPath $destination) { throw "Preserve the existing $Candidate package. Use a fresh checkout or explicitly archive it before packaging again: $destination" }
$ancestor = $destination
while ($ancestor.Length -ge $projectRoot.Length) {
    if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Package paths must not traverse linked directories.' }
    if ($ancestor -eq $projectRoot) { break }
    $ancestor = [IO.Path]::GetDirectoryName($ancestor)
}
$runDirectory = Join-Path $projectRoot ("Saved\Packaging\$Candidate\" + (Get-Date -Format 'yyyyMMdd_HHmmss_fff') + '_' + [Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
$record = [ordered]@{ candidate=$Candidate; status='RUNNING'; maps=$maps; package_directory=$destination; started_utc=[DateTime]::UtcNow.ToString('o'); release_verified=$false; source_runtime_verified=$false; verification_contract=$contract }
$recordPath = Join-Path $runDirectory 'package_creation.json'
$record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding UTF8
try {
    & "$engineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$projectRoot\ProjectONE.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook "-map=$maps" -stage -pak -prereqs -archive "-archivedirectory=$destination" -utf8output -unattended
    $record['uat_exit_code'] = $LASTEXITCODE
    if ($LASTEXITCODE -ne 0) { throw "Package failed: $LASTEXITCODE" }
    $required = @('ProjectONE.exe','ProjectONE/Binaries/Win64/ProjectONE.exe','ProjectONE/Content/Paks/ProjectONE-Windows.pak','ProjectONE/Content/Paks/ProjectONE-Windows.ucas','ProjectONE/Content/Paks/ProjectONE-Windows.utoc','Engine/Extras/Redist/en-us/vc_redist.x64.exe')
    foreach ($relative in $required) {
        $path = Join-Path (Join-Path $destination 'Windows') $relative
        if (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-Item -LiteralPath $path).Length -le 0) { throw "UAT exited successfully but a required runtime/prerequisite is missing or empty: $relative" }
    }
    $record['required_runtime_files_present'] = $required
    $record['status'] = 'PACKAGE_CREATED_PENDING_VERIFICATION'
} catch {
    $record['status'] = 'FAILED'; $record['error'] = $_.Exception.Message
    throw
} finally {
    $record['ended_utc'] = [DateTime]::UtcNow.ToString('o')
    $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding UTF8
}
Write-Output "$Candidate package created; verification remains pending. Record: $recordPath"
