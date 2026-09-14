# Candidate06 remains the advertised default until Candidate07 is verified.
param([ValidateSet('Candidate02','Candidate03','Candidate04','Candidate05','Candidate06','Candidate07')][string]$Candidate = 'Candidate06', [switch]$Sandbox)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$gameExe = Join-Path $projectRoot "Packaged\$Candidate\Windows\ProjectONE.exe"
if (!(Test-Path -LiteralPath $gameExe)) { throw "Packaged $Candidate is missing. Run Scripts/Package.ps1 -Candidate $Candidate for that explicit target; package creation alone is not release verification." }
$arguments = @('-windowed','-ResX=1600','-ResY=900','-ForceRes')
if ($Sandbox) { $arguments = @('/Game/ONE/Maps/Containment?ONESandbox=1') + $arguments }
Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList $arguments -WindowStyle Normal
