# Validate an existing package; this script does not build or curate Evidence.
param(
    [ValidateSet('Candidate02', 'Candidate03')][string]$Candidate = 'Candidate03',
    [string[]]$Modes,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 240,
    [switch]$ShowWindow,
    [switch]$PlanOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$gameExe = Join-Path $projectRoot "Packaged\$Candidate\Windows\ProjectONE.exe"
$runRoot = Join-Path $projectRoot ("Saved\PackagedValidation\$Candidate\" + (Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
$suite = [ordered]@{}
if ($Candidate -eq 'Candidate03') {
    $suite['ONE03MovementCheck'] = 'ONE03_MOVEMENT_COMPLETE'
    $suite['ONE03WeaponCheck'] = 'ONE03_WEAPON_COMPLETE'
    $suite['ONE03CaseCheck'] = 'ONE03_CASE_COMPLETE'
    $suite['ONE03PresentationCheck'] = 'ONE03_PRESENTATION_COMPLETE'
    $suite['ONE03PresentationCapture'] = 'ONE03_PRESENTATION_COMPLETE'
    $suite['ONE03DamageCheck'] = 'ONE03_DAMAGE_COMPLETE'
    $suite['ONE03PhysicalityCheck'] = 'ONE03_PHYSICALITY_COMPLETE'
    $suite['ONE03PhysicalityCapture'] = 'ONE03_PHYSICALITY_COMPLETE'
}
$suite['ONECombatCheck'] = 'ONE_COMBAT_COMPLETE'
$suite['ONECompare'] = 'ONE_COMBAT_COMPLETE'
$suite['ONEPresentation'] = 'ONE_PRESENTATION_COMPLETE'
$suite['ONEValidate'] = 'ONE_VALIDATION_COMPLETE'
foreach ($count in @(6, 12, 18)) { $suite["ONEBenchmark=$count"] = 'ONE_VALIDATION_COMPLETE' }
if (!$Modes) { $Modes = @($suite.Keys | Where-Object { $_ -notin @('ONE03PresentationCapture','ONE03PhysicalityCapture') }) }
if (@($Modes | Select-Object -Unique).Count -ne $Modes.Count) { throw 'Duplicate validation modes are not allowed.' }
foreach ($mode in $Modes) {
    if (!$suite.Contains($mode)) { throw "Unsupported $Candidate validation mode: $mode" }
}
if ($PlanOnly) {
    [ordered]@{
        candidate = $Candidate; executable = $gameExe; local_run_directory = $runRoot
        modes = @($Modes | ForEach-Object { @{ mode = $_; completion_marker = $suite[$_] } })
        timeout_seconds = $TimeoutSeconds
        performance_note = 'Legacy benchmarks retain their single screenshot at 15 seconds; this suite does not enable CSV profiling.'
    } | ConvertTo-Json -Depth 5
    return
}
if (!(Test-Path -LiteralPath $gameExe -PathType Leaf)) { throw "Missing $Candidate package; run Package.ps1 for the intended candidate first." }

function Assert-ProjectPath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if (!$resolved.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Validation output or capture archive escaped project root.'
    }
    # FullPath normalizes '..' but does not resolve junctions. Reject linked
    # ancestors before recursively moving a Saved folder.
    $ancestor = $resolved
    while ($ancestor.Length -ge $projectRoot.Length) {
        if ((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'Validation paths must not traverse linked directories.'
        }
        if ($ancestor -eq $projectRoot) { break }
        $ancestor = [IO.Path]::GetDirectoryName($ancestor)
    }
    return $resolved
}
New-Item -ItemType Directory -Path (Assert-ProjectPath $runRoot) | Out-Null
# Move only this selected package's previous runtime output into a unique local
# backup. Accepted Candidate02 packages and curated Evidence are never selected
# implicitly by the Candidate03 default.
$folders = @('Presentation', 'Validation', 'Candidate02')
if ($Candidate -eq 'Candidate03') { $folders += 'Candidate03' }
foreach ($folder in $folders) {
    $source = Assert-ProjectPath (Join-Path (Split-Path -Parent $gameExe) "ProjectONE\Saved\$folder")
    $destination = Assert-ProjectPath (Join-Path $runRoot "PreviousRuntimeSaved\$folder")
    if (Test-Path -LiteralPath $source) {
        if ((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing to move a linked capture directory.' }
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Move-Item -LiteralPath $source -Destination $destination
    }
}
$results = [Collections.Generic.List[object]]::new()
foreach ($mode in $Modes) {
    $label = $mode.Replace('=', '_')
    $log = Join-Path $runRoot "$label.log"
    $arguments = @("-$mode", '-windowed', '-ResX=1600', '-ResY=900', '-unattended', '-nosplash', "-abslog=`"$log`"")
    # Process-only master-mix capture override; normal game settings stay intact.
    if ($mode -in @('ONECompare', 'ONE03PresentationCapture', 'ONE03PhysicalityCapture')) { $arguments += '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0' }
    $windowStyle = if ($ShowWindow) { 'Normal' } else { 'Hidden' }
    $process = Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList $arguments -WindowStyle $windowStyle -PassThru
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw "$mode exceeded $TimeoutSeconds seconds; inspect the existing game and log before another run: $log" }
    $process.Refresh()
    if ($process.ExitCode -ne 0) { throw "$mode exited with code $($process.ExitCode): $log" }
    if (!(Test-Path -LiteralPath $log -PathType Leaf)) { throw "$mode did not create its run log: $log" }
    $content = Get-Content -LiteralPath $log -Raw
    $marker = [regex]::Escape($suite[$mode])
    if ($content -notmatch ($marker + ' failures=0(?:\s|$)') -or $content -match ($marker + ' failures=[1-9][0-9]*')) {
        throw "$mode did not report its own successful completion: $log"
    }
    $results.Add(@{ mode = $mode; status = 'PASS'; completion_marker = $suite[$mode]; exit_code = $process.ExitCode })
    @{ candidate = $Candidate; all_requested_modes_complete = ($results.Count -eq $Modes.Count); results = @($results.ToArray()) } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runRoot 'summary.json') -Encoding UTF8
    Write-Output "$mode PASS ($log)"
}
