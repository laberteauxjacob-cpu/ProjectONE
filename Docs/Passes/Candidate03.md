# Candidate03 — player playtest corrections

Completed correction pass on `codex/candidate03`, from preserved `candidate02`
(`aa4d55d04bf375bbef41362af77eec10d9ea224f`). Candidate02's package is retained.
The consolidated Candidate03 assignment governs this pass; no duplicate pass,
new map, weapon roster, perk system or infected archetype is being introduced.

## Stage record

- A: actual source, branch, package and direct gameplay baseline inspected.
  See [baseline observations](../../Evidence/Candidate03/baseline.md).
- B: movement, turning, reload intent and weapon access implemented; the
  specific directional and turn defects passed rendered sequence review.
  Editable tuning: walk 225 cm/s,
  sprint 370, infected pursuit 195. All 48 directional speed trials passed
  (both weapons, eight directions, walk/sprint/reload walk), alongside 85
  weapon checks including frame-spaced production input bindings.
- C: weapon flash, audio and visible case improvements implemented. The corrected
  rendered run passed 121 presentation assertions and 49 separate case checks;
  final sequence review is recorded below. Perceptual audio audition is unavailable.
- D: ragdoll, broader regional severing, bleeding and contact implemented;
  244/244 rendered integration checks and 42/42 damage checks passed. Focused
  motion review covers the freeze/resume transitions and visible minor trails.
- E: fresh exact-source package, all 13 packaged modes, final physicality and
  native input recordings, normal launch controls, four performance profiles,
  archive audit and publication. Final results and limitations follow below.

Implementation, automated checks, bounded motion inspection, engine audio
measurement and ordinary packaged input remain separate evidence. Visual
direction and perceptual sound quality are not marked approved by these checks.

## Stage B implementation and evidence

The eight walk and eight run clips use actual speed and authored stride length;
the old diagonal phase multiplier and sprinting side/back walk substitutions are
removed. Two stepping turn clips, independent upper-body aim and world-space
foot preservation handle stationary turning. Skeletal evaluation waits for the
player's current movement/aim update. The final focused turn capture corrected
an earlier 18 cm one-frame foot snap on abrupt 180-degree aim changes. Continuous
90/180-degree-per-second sweeps, settled jumps and mid-step reversals were
captured separately from the earlier complete directional run.

Shift takes precedence over manual and automatic reload requests, including
while stationary. Reload cancellation processes only legitimately elapsed
ammunition events, clears obsolete presentation and buffered fire, and preserves
shotgun pump/ejection obligations. An empty equipped weapon with reserve reloads
once eligible; held Shift, pause, death and incompatible operations defer it.
The two slots now say **5.56 mm Carbine** and **12-Gauge Pump Shotgun**, with
1/2, Tab and wheel selection displayed together.

These are editor-game integration results, not final packaged results. Native
ordinary input verified weapon selection, mouse fire/aim, reload, pause and
quit. The available native automation delivered W taps within one engine frame
and did not deliver Shift key edges; sustained movement and Shift interruption
are therefore covered by automated production bindings and movement probes,
not claimed as native held-key playtesting. Directional motion review still
identifies deep knee compression and a mechanical cadence as provisional art.
Audio has not been perceptually auditioned.

## Stage C implementation and evidence

The flash is a short tapered volume attached to the actual gun muzzle, updated
with the existing light after current skeletal evaluation. The old broad
world-space origin ribbon is removed; the thin shot trajectory remains separate.
F7 switches existing room lights between bright and 18% dim intensity without
changing exposure. The new material importer validates its connections and
reads the graph back after compilation; this corrected a defect found in the
first visual recording despite its passing transform checks.

Six original firing profiles per weapon vary attack, body, mechanics and tails;
the actual runtime chooses them without immediate repeats and without pitch
randomization. Distinct original rifle brass and the existing shotgun hull eject
on discharge and pump extraction respectively. Bounded world-space cases inherit
movement, spin, sweep the room, bounce and settle, with 32 retained for six seconds.
They do not block characters or navigation.

See [presentation review](../../Evidence/Candidate03/StageC/presentation_review.md)
and [measurements](../../Evidence/Candidate03/StageC/presentation_metrics.json).
The recording contains genuine engine audio, but numerical sound measurements
do not establish perceptual quality. Packaged checks were pending at this Stage C
checkpoint; their final exact-source results appear below.

The unchanged Candidate02 baseline was profiled at 1600x900 with 6/12/18 enemies.
[Timing reports](../../Evidence/Candidate03/Performance/C02/summary.json) retain
the benchmark's screenshot-associated spikes and separate individual Chaos CPU
scopes from frame duration. Candidate03 comparison follows the final package.

## Stage D implementation and editor-game evidence

The accepted infected skeleton now drives a capped core and separate head,
anatomical left/right arms and left leg. The import reflects the source sides:
source `_r` is anatomical left, and `_l` is anatomical right. The editable
derived Blender source preserves the accepted geometry outside the authored
cuts. Right-leg hits contribute damage, but right-leg removal is not implemented.
Head loss, left-leg loss or loss of both arms is fatal. A single missing arm
selects the authored attack for the remaining arm.

Each weapon discharge accumulates damage, trauma, pellet count and spatial
information independently for all six regions before resolving one victim
transaction. A per-victim cache rejects repeated nonzero IDs among the last 32
accepted discharges; the registered live set separately prevents duplicate kill
awards. Fresh
corpse hits can create wounds, impulses and additional eligible severing without
awarding another kill. Arm health damage remains scaled by 0.4; head, arm and
left-leg sever thresholds are 32, 50 and 70 trauma respectively.

Death captures the evaluated skeletal pose before stopping movement and uses
bounded inherited velocity and impact impulses. A source-generated physics asset
contains constrained body shapes; detached pieces use matching smaller assets.
Severed chains are terminated, and the retained left-thigh stump transfers its
collision coverage to a capsule fitted to the captured pose. Ragdolls and parts
collide with the room and each other while ignoring character capsules and
navigation. Physical settling and visual continuity are separate runtime gates.

Wounds use finite visual blood budgets rather than medical volume units. Minor
wounds emit briefly; severe torso wounds and severing emit more heavily and for
longer. Gravity-driven droplets project onto room surfaces and merge compatible
stains into gradually growing pools. Torso damage adds a surface wound, not a
modeled chest opening. Limits are 96 wounds, 48 in-flight droplets, 90 decals,
14 corpses and 18 detached pieces, with at most 12 projection traces in a frame.
Round-robin scheduling and a generation-stamped cleanup prevent old emitters
from restarting after an encounter reset.

Living capsules retain their 27 cm radius. Avoidance radius inflation is reduced
from 1.5 to 1.1, and simulated corpse contact cannot push or trap the player.
The integration scenario covers a room corner, the side of the rack, the bench
end, several moving deaths and a sustained mixed living/dead encounter. These
are connected to ordinary gameplay and covered by the runtime results below.

The crowd fixtures exposed stale collision in the existing room kit. Each of
eight meshes retained seven convex hulls, including old rotated hulls extending
far beyond the visible walls and benches. This split the floor into isolated
navigation regions. The targeted repair replaces those hulls with one NDOP18
per mesh, aligns the serialized navigation agent with 32/185 cm configuration,
waits for navigation to finish and saves the same room layout. The repaired
corner, rack and bench fixtures each spawned all 18 enemies, all advanced more
than one metre, and attacks reached the player. Closest capsule spacing was
54 cm with no measured overlap. See the
[collision and navigation comparison](../../Evidence/Candidate03/StageD/navigation_comparison.md).

The first full rendered integration run passed 237 of 238 assertions but failed
the six-corpse settling check. Consecutive late frames confirmed a small twitch
in the pile. A second run with bounded 1/120-second physics substeps reduced
most motion but retained that failure; Stage D was incomplete at that checkpoint.
Per-corpse motion diagnostics distinguished an actually moving body from
quiet bodies sharing its awake contact island. The later correction and fourth
capture are recorded below. Global gravity is unchanged.

The same review found that numerical pool growth did not establish visible
growth. Pool transform updates now retain their original fade timing; the old
render-state recreation restarted fade-in during growth. A separate pool
material resamples the continuous centre of the original blood mask, whose
small central footprint was largely concealed by a corpse. The complete mask
remains on impact and wound presentation. This introduces no external texture
or new bleeding budget. The third rendered run's 85-frame pool sequence and
native views confirmed visible growth around the torso. The radius factor
remains six; no extra enlargement was needed. Stamp-like edges remain provisional.

The third run also proved that a supported sleep request was immediately undone
by another moving corpse in the shared Chaos contact island. The revised guard
requires at least two seconds since disturbance, every active body below 5 cm/s
and 0.35 rad/s, every simulated body within 1 cm and 2 degrees of its fixed measured
window pose for 1.25 seconds,
and confirmed static support within 3 cm. Only then does it preserve that evaluated
pose as kinematic bodies with collision retained. Natural sleeping remains valid.
Fresh corpse damage resumes the retained bodies from their current pose before
applying the impulse; missing chains remain terminated. Freeze and resume
continuity are measured separately. The fourth full rendered run passed all
244 assertions, including a naturally qualifying corpse that froze, resumed
on one fresh hit, and qualified again afterward. Its resume error was 0 cm
and 0.041 degrees; no additional health or kill award occurred. All six final
pelvis orientations differed by more than five degrees. At the end, 64 of the
original 96 bodies remained awake, 16 slept naturally and 16 were kinematic;
this is not a claim that every limb was completely still. The reviewed late
pile retained small hand/forearm adjustments among unfrozen members.

The final late-sever regression after this integration passed all 42 assertions:
actual anatomical queries, remaining-arm attacks, regional pellet transactions,
no duplicate awards, evaluated-pose detachment and retained stump continuity.
The optional frozen-corpse hit probe is part of the separate rendered scenario.

See the [244-check report](../../Evidence/Candidate03/StageD/editor_physicality_checks.txt),
[damage report](../../Evidence/Candidate03/StageD/editor_damage_checks.txt),
[observations](../../Evidence/Candidate03/StageD/editor_physicality_observations.csv)
and [motion review](../../Evidence/Candidate03/StageD/visual_review.md).
These are editor-game working-tree runs. The final package and performance
comparison must be verified separately from the exact public gameplay commit.

## Stage E first source build

The first integrated source was
[`6c7a00c325eb3f4b43b42f186e1d04c2e07fee8f`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/6c7a00c325eb3f4b43b42f186e1d04c2e07fee8f).
It was pushed to `codex/candidate03` after the outgoing-source audit passed.
A fresh public clone in a neutral build directory fetched that commit, verified
569 tracked files and all 291 current LFS payloads, and confirmed that the private
original history was absent. Editor, Win64 game and complete Development package
builds passed using UE 5.7.2. Required runtime bytes are bound to that source hash.

All five native combat tests completed successfully. Three tests produced five
`World has no context` warnings while destroying actors in isolated temporary
test worlds; these are retained in the verification record. The native result is
two successes without warnings, three successes with warnings, and zero failures.
The read-only release audit passed with no unclassified personal paths; runtime
executables were not patched after compilation. The combined packaged suite,
native-input check, final motion capture and performance comparison were separate
pending gates at that point.

## Stage E final source build and fixture correction

The final gameplay/package source is
[`e81e1137ed891570c73c8c278fc2c8cc2250bd04`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/e81e1137ed891570c73c8c278fc2c8cc2250bd04).
Only `ONEValidation.cpp` changed after the first integrated source. Its stopped
packaged suite passed nine modes before the old one-arm pursuit assertion failed;
the three following benchmarks were not run. That result is preserved in the
[first-source record](../../Evidence/Candidate03/StageE/first_source_validation.md).

A rendered diagnostic measured 88.565 cm of initial approach, then 35.786 cm of
net displacement from spawn after following a moving player. The infected was
alive and 59.120 cm from the player. Returning toward its original location had
invalidated the old endpoint-as-travel assumption. The revised fixture retains
the 40 cm movement threshold during the stationary-target approach, separately
checks continued proximity, and requires survival and correct remaining-arm
state at both observations. Its scripted gunfire now uses a fixed perpendicular
lane so desktop cursor position cannot aim at its own pursuit subject. This
changes the validation driver, not normal gameplay or weapon damage. The final
local rendered retest passed 35 assertions; the exact-source packaged result is
recorded separately.

The clean neutral public checkout advanced to that published source, reverified
all 291 current LFS payloads and rebuilt editor, game and complete Windows
Development package successfully. Build times were 14.188, 19.766 and 20.734
seconds respectively. All five native combat tests passed again, including the
same five temporary-test-world teardown warnings across three tests. The
[fresh-build record](../../Evidence/Candidate03/StageE/fresh_build.json) binds the
six required runtime/prerequisite hashes to the exact source.

The [release audit](../../Evidence/Candidate03/StageE/release_audit.json) verified
45 retained runtime files totaling 653,635,299 bytes, with no findings and no
post-build executable patching. Its 614 excluded staged files include accumulated
local test output under `Saved`, debug symbols and staging manifests; these are
separate from the archive's unused third-party-notice exclusions. Original S1
build and audit records remain byte-preserved beside the final-source reports.
The following final-source records supersede those pending gates without
rewriting the earlier S1 failure or Stage D editor captures.

## Final packaged regression and motion evidence

All 13 default packaged modes passed from the clean S2 package: 714 logged PASS
assertions, zero FAIL assertions, errors, fatals or ensures. These are logged
assertions, including repeated timed checks, rather than 714 distinct test cases.
The [suite record](../../Evidence/Candidate03/StageE/packaged_checks.json) retains
208 warnings: 192 initial socket lookups before mesh setup and 16 missing-Recast
crowd-manager warnings. Their presence is disclosed separately from the passing
navigation assertions. The final suite had no critical audio callback warning.

The final physicality recording passed 249 assertions and saved 3,069 original
frames. Its review covers 430 dense frame presentations, 36 sparse context
frames and eight native-pixel rechecks in phases 2–5 (446 distinct images),
333 dense blood/crowd/cleanup presentations plus 11 native checks (337 distinct),
and 60 additional consecutive frames around resume and late settling. These
are bounded chronological image inspections, not a claim of viewing every frame.
See the [motion](../../Evidence/Candidate03/StageE/physicality_motion_review.md),
[blood/crowd](../../Evidence/Candidate03/StageE/physicality_blood_crowd_review.md)
and [rest](../../Evidence/Candidate03/StageE/physicality_rest_review.md) records.

Both arm paths visibly separate and retain use of the available arm before
collapse. Head separation is clear; the leg-loss collapse is visible, with some
part/contact details occluded. Six moving deaths produce different articulated
results. A naturally qualified frozen corpse resumed on a fresh accepted hit
with measured continuity of 0 cm and 0.038784 degrees. Phase 5 ended with 32
awake body instances out of a 96-instance peak and three frozen corpses. This
is not a claim that every body sleeps. No per-body late-velocity telemetry was
recorded in this final capture. Quiet late silhouettes coexist with awkward
bent/kneeling forms and overlapping contacts in the upper pile.

The same-source legacy comparison captured 20.992 seconds of genuine engine
master audio at 48 kHz stereo PCM16. Both requested weapon windows were fully
covered and non-silent; peak was −13.889 dBFS with zero full-scale samples.
Its 51 detected energy groups are not a shot count. The
[audio record](../../Evidence/Candidate03/StageE/packaged_comparison_audio.json)
documents numerical output and coverage. Perceptual audition remains unavailable.

## Final performance comparison

Four serial rendered profiles completed without audio/video recording or
concurrent build/encoding work. Matched living-crowd runs used the recorded
Windows Development/UE 5.7.2, Ryzen 9 5950X, RTX 3090, 1600×900, quality 2,
100% screen percentage, FXAA, disabled GI/reflections, VSync off and 120 FPS cap.
Each captured 3,000 frames and verified 6/12/18 living enemies at the end.
The primary CSV-relative 10–20-second interval retains the existing screenshot
spike on both builds. Cells below are Candidate02 → Candidate03, milliseconds.

| Living count | Mean | p99 | Maximum |
| --- | --- | --- | --- |
| 6 | 8.4356 → 8.4330 | 8.6117 → 8.6609 | 135.5696 → 132.6865 |
| 12 | 8.4410 → 8.4376 | 8.7093 → 8.7235 | 141.3121 → 136.6046 |
| 18 | 8.4418 → 8.4411 | 8.7933 → 9.0602 | 142.3466 → 140.2602 |

The `PhysicsVerbose/AllWorkers/StepSolver` mean increased from 0.1532/0.1801/
0.2081 ms to 0.2535/0.3211/0.4351 ms at those counts. Nested worker scopes
overlap and must not be summed into physics wall time. The capped average
alone would hide this increased CPU work.

The separate eleven-phase physicality profile passed 257 assertions with zero
failures and zero media frames. It retained 14,038 frames over 117.0604 seconds:
mean 8.3388 ms, p99 9.0456 ms, maximum 31.4258 ms, five frames above 16.7 ms and
none above 33.3 ms. Phase 5 StepSolver mean/p99/max was 0.8828/1.3680/1.7572 ms;
mixed phase 9 was 1.5650/2.2371/3.1371 ms. The full-run project RagdollRest
scope measured mean 0.001707 ms, p99 0.0274 ms and max 0.3240 ms; BloodFixedStep
was 0.003166/0.0895/0.4592 ms. These include zero-execution frames and are not
single-operation latency or the entire timer's wall cost.

An existing unsaved Blender GUI remained untouched. Identity-bound five-second
preflights and per-scenario before/after CPU accounting stayed below the fixed
0.05 CPU-second-per-wall-second ceiling. The measured scenario rates were
0.0066–0.0093, relative to one logical CPU. Brief bursts cannot be excluded;
Candidate02 ambient CPU was not recorded identically. One repeat per count,
foreground/power/background uncertainty and changed systems prevent causal or
capacity claims. The [performance report](../../Evidence/Candidate03/Performance/README.md)
contains full runs, spike counts, per-phase costs and four complete numerical
timelines with exact selections.

## Final ordinary input and delivery

The extracted archive's executable was operated with native mouse/key input.
Its observer recorded 4,970 frames and genuine engine audio: two carbine shots,
two shotgun shots, both manual reloads, 1/2/Tab/both wheel directions, bright/dim
lighting, refill, pause/resume and a spawned infected. That infected killed the
stationary player; the later LMB and R inputs left ammunition unchanged after
death. The recorder's zero-failure marker is completion telemetry, not a count
of functional assertions. Its full input record and scope are in the
[native review](../../Evidence/Candidate03/StageE/native_input_review.md).

A separate ordinary launch through `.\Scripts\Launch.ps1 -Candidate Candidate03`
used no recording or validation flags. It started round 1 with six infected,
entered sandbox through F1, spawned six through F3, reset through F5, accepted
F6 cleanup, returned to rounds through F1, restarted through Escape/Enter and
quit through Escape/Q. Full inventory and six round-1 infected returned without
a stray shot. F6 was exercised after the case lifetime elapsed with no populated
remains; the separate physicality scenario verifies populated cleanup. The
six-infected native encounter also logged player death just before F5 reset.
These observations do not claim successful native combat or held WASD/Shift.

The five [review movies](../../Evidence/Candidate03/README.md) preserve their
actual editor-game or final packaged provenance. Dense source callbacks become
30 FPS movies by holding/dropping genuine frames against the engine audio clock,
without interpolated motion or replacement audio. The physicality movie excludes
one 25.596 ms out-of-audio end callback through an explicitly recorded subset;
all original frames remain private. Its eleven chapter titles were read back
after an AV-packet-preserving metadata remux. Native media has its own adjacent
assembly, chapter and audio records. None received perceptual audio approval.

The complete Windows archive is **398,043,452 bytes**, with **1,217 entries**,
SHA-256 `ada689b2314d3047d581d0523a2cbcd91f57661d06554d6288a6b4fae79600d2`.
Its [audit](../../Evidence/Candidate03/StageE/archive_audit.json) checks every
entry CRC and manifest payload against the fresh source build, including both
Windows prerequisite installers and retained runtime notices. No executable
was patched. The archive was extracted into a previously absent Candidate03
package directory and its runtime hashes verified before native playtesting.
[Preservation](../../Evidence/Candidate03/StageE/preservation_check.json)
rechecks Candidate02 and the exact archive/runtime bytes after native testing.

- [Repository](https://github.com/laberteauxjacob-cpu/ProjectONE)
- [Review branch](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/codex/candidate03)
- [Immutable candidate03 reference](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate03)
- [Prerelease and checksum sidecars](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
- [Complete playable archive](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate03/ProjectONE-Candidate03-Windows.zip)
- [Candidate02 comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate02...candidate03)

The final candidate reference contains documentation and evidence after built
source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`; it does not identify a different
gameplay build. The release notes name both revisions. Prior candidate references
remain immutable. Source assets/generation scripts remain editable through LFS;
raw logs, dense regenerable captures and personal diagnostics stay outside Git.
The release's publication-verification attachment records the later remote
reference, LFS and downloaded-asset checks without changing this immutable tag.

The next recommended milestone is a focused locomotion/turn and corpse-pile
visual polish pass in this arena, guided by the user's playtest and audio
feedback. Deep knee compression, mechanical strides, awkward propped/interleaved
remains, occluded contacts, subtle cases and black off-map corner space remain
visible limitations. Right-leg severing is absent; frozen supported corpses
resume on fresh damage, not ordinary contact. No new milestone starts here.
