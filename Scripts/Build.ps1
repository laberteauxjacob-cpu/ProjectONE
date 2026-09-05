param([switch]$Game, [string]$EngineRoot = $env:UE_ROOT)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
. "$PSScriptRoot\Engine.ps1"
$engineRoot = Resolve-ONEEngine -EngineRoot $EngineRoot
$target = if ($Game) { 'ProjectONE' } else { 'ProjectONEEditor' }
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" $target Win64 Development "-Project=$projectRoot\ProjectONE.uproject" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
