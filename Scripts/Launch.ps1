$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$gameExe = Join-Path $projectRoot 'Packaged\Candidate02\Windows\ProjectONE.exe'
if (!(Test-Path -LiteralPath $gameExe)) { throw 'Packaged candidate is missing. Run Scripts/Package.ps1 first.' }
Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList '-windowed','-ResX=1600','-ResY=900' -WindowStyle Normal
