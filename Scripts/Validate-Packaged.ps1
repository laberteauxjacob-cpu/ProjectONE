# Validate an existing package; this script does not build or curate Evidence.
param(
    [ValidateSet('Candidate02', 'Candidate03', 'Candidate04', 'Candidate05', 'Candidate06', 'Candidate07')][string]$Candidate = 'Candidate06',
    [string[]]$Modes,
    [string]$Map,
    [ValidateRange(30, 600)][int]$TimeoutSeconds = 450,
    [switch]$ShowWindow,
    [switch]$RenderOffscreen,
    [switch]$PlanOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$gameExe = Join-Path $projectRoot "Packaged\$Candidate\Windows\ProjectONE.exe"
$runRoot = Join-Path $projectRoot ("Saved\PackagedValidation\$Candidate\" + (Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
$suite = [ordered]@{}
if ($Candidate -in @('Candidate03','Candidate04','Candidate05','Candidate06','Candidate07')) {
    $suite['ONE03MovementCheck'] = 'ONE03_MOVEMENT_COMPLETE'
    $suite['ONE03WeaponCheck'] = 'ONE03_WEAPON_COMPLETE'
    $suite['ONE03CaseCheck'] = 'ONE03_CASE_COMPLETE'
    $suite['ONE03PresentationCheck'] = 'ONE03_PRESENTATION_COMPLETE'
    $suite['ONE03PresentationCapture'] = 'ONE03_PRESENTATION_COMPLETE'
    $suite['ONE03DamageCheck'] = 'ONE03_DAMAGE_COMPLETE'
    $suite['ONE03PhysicalityCheck'] = 'ONE03_PHYSICALITY_COMPLETE'
    $suite['ONE03PhysicalityCapture'] = 'ONE03_PHYSICALITY_COMPLETE'
}
if ($Candidate -in @('Candidate04','Candidate05','Candidate06','Candidate07')) {
    $suite['ONE04ProgressionCheck'] = 'ONE04_PROGRESSION_COMPLETE'
    $suite['ONE04ArsenalCheck'] = 'ONE04_ARSENAL_COMPLETE'
}
if ($Candidate -in @('Candidate05','Candidate06','Candidate07')) {
    $suite['ONE05AimCheck'] = 'ONE05_AIM_COMPLETE'
    $suite['ONE05WeaponCheck'] = 'ONE05_WEAPON_COMPLETE'
    $suite['ONE05MotionCheck'] = 'ONE05_MOTION_COMPLETE'
    $suite['ONE05UICheck'] = 'ONE05_UI_COMPLETE'
}
if ($Candidate -in @('Candidate06','Candidate07')) {
    $suite['ONE06CombatCheck'] = 'ONE06_COMBAT_COMPLETE'
    $suite['ONE06GroupingCheck'] = 'ONE06_GROUPING_COMPLETE'
    $suite['ONE06SurvivalCheck'] = 'ONE06_SURVIVAL_COMPLETE'
    $suite['ONE06PortabilityCheck'] = 'ONE06_PORTABILITY_COMPLETE'
}
if ($Candidate -eq 'Candidate07') {
    $suite['ONE07PhysicalityCheck'] = 'ONE07_PHYSICALITY_COMPLETE'
    $suite['ONE07Encounter'] = 'ONE07_PHYSICALITY_COMPLETE'
}
$suite['ONECombatCheck'] = 'ONE_COMBAT_COMPLETE'
$suite['ONECompare'] = 'ONE_COMBAT_COMPLETE'
$suite['ONEPresentation'] = 'ONE_PRESENTATION_COMPLETE'
$suite['ONEValidate'] = 'ONE_VALIDATION_COMPLETE'
foreach ($count in @(6, 12, 18)) { $suite["ONEBenchmark=$count"] = 'ONE_VALIDATION_COMPLETE' }
if (!$Modes) {
    if ($Candidate -eq 'Candidate07') { $Modes = @('ONE07PhysicalityCheck') }
    else { $Modes = @($suite.Keys | Where-Object { $_ -notin @('ONE03PresentationCapture','ONE03PhysicalityCapture','ONE06PortabilityCheck') }) }
}
if (@($Modes | Select-Object -Unique).Count -ne $Modes.Count) { throw 'Duplicate validation modes are not allowed.' }
foreach ($mode in $Modes) {
    if (!$suite.Contains($mode)) { throw "Unsupported $Candidate validation mode: $mode" }
}
if ($Modes -contains 'ONE06PortabilityCheck') {
    if ($Map -cne '/Game/ONE/Maps/Portability06') { throw 'ONE06PortabilityCheck requires explicit -Map /Game/ONE/Maps/Portability06.' }
} elseif ($Map) { throw '-Map is reserved for the explicit ONE06PortabilityCheck mode; other modes keep their normal map.' }
if ($PlanOnly) {
    [ordered]@{
        candidate = $Candidate; executable = $gameExe; local_run_directory = $runRoot
        modes = @($Modes | ForEach-Object { @{ mode = $_; completion_marker = $suite[$_]; map = $(if ($_ -eq 'ONE06PortabilityCheck') { $Map } else { 'project default' }) } })
        timeout_seconds = $TimeoutSeconds
        timeout_note = 'The outer process limit includes startup/shutdown; ONE06CombatCheck has its own 180-second actor limit and ONE06SurvivalCheck a 140-second real-time actor limit.'
        render_mode = $(if ($RenderOffscreen) { 'offscreen engine viewport' } else { 'native window' })
        performance_note = 'Legacy benchmarks retain their single screenshot at 15 seconds; this suite does not enable CSV profiling.'
        verification_contract = 'Only the explicitly requested runtime modes are checked. No fresh-source, complete-release, media, native-input, audio-audition, profile or public-download result is established.'
        source_runtime_verified = $false; release_verified = $false
    } | ConvertTo-Json -Depth 5
    return
}
if (!(Test-Path -LiteralPath $gameExe -PathType Leaf)) { throw "Missing $Candidate package; run Package.ps1 for the intended candidate first." }
if (Get-Process -Name 'ProjectONE*' -ErrorAction SilentlyContinue) { throw 'An existing ProjectONE game is running. Close it before validation or moving its prior Saved output.' }

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
# backup. Earlier candidate packages and curated Evidence are never selected
# implicitly by the Candidate06 default.
$folders = @('Presentation', 'Validation', 'Candidate02')
if ($Candidate -in @('Candidate03','Candidate04','Candidate05','Candidate06','Candidate07')) { $folders += 'Candidate03' }
if ($Candidate -in @('Candidate04','Candidate05','Candidate06','Candidate07')) { $folders += 'Candidate04' }
if ($Candidate -in @('Candidate05','Candidate06','Candidate07')) { $folders += 'Candidate05' }
if ($Candidate -in @('Candidate06','Candidate07')) { $folders += 'Candidate06' }
if ($Candidate -eq 'Candidate07') { $folders += 'Candidate07' }
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
function Write-SuiteSummary([string]$State) {
    @{ candidate=$Candidate; status=$State; requested_modes=$Modes; all_requested_modes_complete=($State -eq 'REQUESTED_MODES_PASS'); source_runtime_verified=$false; release_verified=$false; results=@($results.ToArray()); scope='Requested runtime modes only; independent source/runtime, release, media, profile and publication gates remain separate.' } |
        ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $runRoot 'summary.json') -Encoding UTF8
}
Write-SuiteSummary 'RUNNING'
foreach ($mode in $Modes) {
    $label = $mode.Replace('=', '_')
    $log = Join-Path $runRoot "$label.log"
    $arguments = @("-$mode", '-windowed', '-ResX=1600', '-ResY=900', '-unattended', '-nosplash', "-abslog=`"$log`"")
    if ($mode -eq 'ONE06PortabilityCheck') { $arguments = @($Map) + $arguments }
    # Offscreen viewports must retain the requested test size even when the
    # desktop work area is smaller or changes during an unattended run.
    if ($RenderOffscreen) { $arguments += @('-RenderOffScreen', '-ForceRes') }
    # Process-only master-mix capture override; normal game settings stay intact.
    if ($mode -in @('ONECompare', 'ONE03PresentationCapture', 'ONE03PhysicalityCapture')) { $arguments += '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0' }
    $windowStyle = if ($ShowWindow) { 'Normal' } else { 'Hidden' }
    try {
        $process = Start-Process -FilePath $gameExe -WorkingDirectory (Split-Path -Parent $gameExe) -ArgumentList $arguments -WindowStyle $windowStyle -PassThru
        if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw "$mode exceeded $TimeoutSeconds seconds; inspect the existing game and log before another run: $log" }
        $process.Refresh()
        if ($process.ExitCode -ne 0) { throw "$mode exited with code $($process.ExitCode): $log" }
        if (!(Test-Path -LiteralPath $log -PathType Leaf)) { throw "$mode did not create its run log: $log" }
        $content = Get-Content -LiteralPath $log -Raw
        $marker = [regex]::Escape($suite[$mode])
        $completion = [regex]::Matches($content, '(?m)\b' + $marker + '\b(?<fields>[^\r\n]*)')
        if ($completion.Count -ne 1) { throw "$mode needs exactly one own completion marker: $log" }
        $fields = @{}
        foreach ($pair in [regex]::Matches($completion[0].Groups['fields'].Value, '\b([A-Za-z_]+)=([0-9]+)(?=\s|$)')) {
            if ($fields.ContainsKey($pair.Groups[1].Value)) { throw "Duplicate completion field: $log" }
            $fields[$pair.Groups[1].Value] = [int64]$pair.Groups[2].Value
        }
        foreach ($key in @('failures','complete','checks','encounter')) {
            $declared = [regex]::Matches($completion[0].Groups['fields'].Value, '\b' + $key + '=')
            if ($declared.Count -gt 1 -or ($declared.Count -eq 1 -and !$fields.ContainsKey($key))) { throw "Malformed or duplicate completion field: $log" }
        }
        if (!$fields.ContainsKey('failures') -or $fields['failures'] -ne 0 -or ($fields.ContainsKey('complete') -and $fields['complete'] -ne 1) -or ($fields.ContainsKey('checks') -and $fields['checks'] -le 0)) {
            throw "$mode did not report its own successful completion: $log"
        }
        if ($mode -in @('ONE07PhysicalityCheck','ONE07Encounter')) {
            $expectedEncounter = if ($mode -eq 'ONE07Encounter') { 1 } else { 0 }
            if (!$fields.ContainsKey('encounter') -or $fields['encounter'] -ne $expectedEncounter -or !$fields.ContainsKey('checks')) { throw "Candidate07 completion belongs to a different mode or lacks checks: $log" }
        }
        $results.Add(@{ mode=$mode; status='PASS'; completion_marker=$suite[$mode]; completion_fields=$fields; exit_code=$process.ExitCode; log_file=[IO.Path]::GetFileName($log); log_sha256=(Get-FileHash -LiteralPath $log -Algorithm SHA256).Hash.ToLowerInvariant() })
        $state = if ($results.Count -eq $Modes.Count) { 'REQUESTED_MODES_PASS' } else { 'RUNNING' }
        Write-SuiteSummary $state
        Write-Output "$mode PASS ($log)"
    } catch {
        $results.Add(@{ mode=$mode; status='FAILED'; error=$_.Exception.Message; log_file=[IO.Path]::GetFileName($log) })
        Write-SuiteSummary 'FAILED'
        throw
    }
}
