param([string]$EngineRoot = $env:UE_ROOT, [ValidateSet('Candidate03','Candidate04','Candidate05','Candidate06')][string]$Candidate = 'Candidate06')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. "$PSScriptRoot\Engine.ps1"
$engineRoot = Resolve-ONEEngine -EngineRoot $EngineRoot
$maps = if ($Candidate -eq 'Candidate06') { '/Game/ONE/Maps/Containment+/Game/ONE/Maps/Portability06' } else { '/Game/ONE/Maps/Containment' }
& "$engineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$projectRoot\ProjectONE.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook "-map=$maps" -stage -pak -prereqs -archive "-archivedirectory=$projectRoot\Packaged\$Candidate" -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Package failed: $LASTEXITCODE" }
