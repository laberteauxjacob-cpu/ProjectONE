param([ValidateSet('Candidate02','Candidate03','Candidate04','Candidate05')][string]$Candidate = 'Candidate05', [switch]$Sandbox)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$gameExe = Join-Path $projectRoot "Packaged\$Candidate\Windows\ProjectONE.exe"
if (!(Test-Path -LiteralPath $gameExe)) { throw 'Packaged candidate is missing. Run Scripts/Package.ps1 first.' }
$arguments = @('-windowed','-ResX=1600','-ResY=900')
if ($Sandbox) { $arguments = @('/Game/ONE/Maps/Containment?ONESandbox=1') + $arguments }
Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList $arguments -WindowStyle Normal
