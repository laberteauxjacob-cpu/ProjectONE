# Candidate03 — player playtest corrections

Work in progress on `codex/candidate03`, from preserved `candidate02`
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
- E: combined packaged validation, motion/audio evidence and publication pending.

The final report will distinguish implementation, automatic checks, motion
inspection, perceptual listening and ordinary packaged input. No Candidate03
package or public release is claimed at this stage.

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
do not establish perceptual quality. Candidate03 packaged checks remain pending.

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
transaction. Replayed discharge IDs cannot repeat damage or scoring. Fresh
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
most motion but retained that failure; this is not a completed Stage D gate.
Per-corpse motion diagnostics now distinguish an actually moving body from
quiet bodies sharing its awake contact island. Settling corrections are under
test. Global gravity is unchanged.

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
and 0.35 rad/s, all bones within 1 cm and 2 degrees of a fixed pose for 1.25 seconds,
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
