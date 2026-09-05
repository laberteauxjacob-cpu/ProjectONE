param([string]$EngineRoot = $env:UE_ROOT)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. "$PSScriptRoot\Engine.ps1"
$engineRoot = Resolve-ONEEngine -EngineRoot $EngineRoot
& "$engineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$projectRoot\ProjectONE.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Game/ONE/Maps/Containment -stage -pak -archive "-archivedirectory=$projectRoot\Packaged\Candidate01" -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Package failed: $LASTEXITCODE" }
