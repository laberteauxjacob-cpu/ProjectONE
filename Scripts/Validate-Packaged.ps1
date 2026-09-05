$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$gameExe = Join-Path $projectRoot 'Packaged\Candidate02\Windows\ProjectONE.exe'
if (!(Test-Path -LiteralPath $gameExe)) { throw 'Run Package.ps1 first.' }
# Preserve previous captures without mixing their timestamps into the new sequence.
$archiveRoot = Join-Path $projectRoot ('Saved\PreviousPackagedRun_' + (Get-Date -Format 'yyyyMMdd_HHmmss'))
foreach ($folder in @('Presentation', 'Validation', 'Candidate02')) {
    $source = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $gameExe) "ProjectONE\Saved\$folder"))
    $destination = [IO.Path]::GetFullPath((Join-Path $archiveRoot $folder))
    if (!$source.StartsWith($projectRoot + '\') -or !$destination.StartsWith($projectRoot + '\')) { throw 'Capture archive escaped project root.' }
    if (Test-Path -LiteralPath $source) {
        New-Item -ItemType Directory -Path $archiveRoot -Force | Out-Null
        Move-Item -LiteralPath $source -Destination $destination
    }
}
# Run separately: capture writes affect frame timing, so benchmarks never record a sequence.
foreach ($mode in @('ONECombatCheck', 'ONECompare', 'ONEPresentation', 'ONEValidate', 'ONEBenchmark=6', 'ONEBenchmark=12', 'ONEBenchmark=18')) {
    $label = $mode.Replace('=', '_')
    $log = Join-Path $projectRoot "Saved\Logs\Packaged_$label.log"
    $arguments = @("-$mode", '-windowed', '-ResX=1600', '-ResY=900', '-unattended', '-nosplash', "-abslog=`"$log`"")
    # Keep the recorded master mix audible when capture runs without window focus.
    # This process-only override does not change normal gameplay audio behavior.
    if ($mode -eq 'ONECompare') { $arguments += '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0' }
    $process = Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList $arguments -WindowStyle Normal -PassThru
    if (!$process.WaitForExit(150000)) { throw "$mode exceeded 150 seconds; inspect the game and log." }
    $content = Get-Content -LiteralPath $log -Raw
    if ($content -notmatch 'ONE_(VALIDATION|PRESENTATION|COMBAT)_COMPLETE failures=0') { throw "$mode did not report successful completion: $log" }
    Write-Output "$mode PASS ($log)"
}
