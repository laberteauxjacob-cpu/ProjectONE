# Current state — Candidate03

Candidate03 continues the existing containment arena, two carried weapons, one
infected archetype and round/sandbox modes. Its exact verified gameplay/package
source is
[`e81e1137ed891570c73c8c278fc2c8cc2250bd04`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/e81e1137ed891570c73c8c278fc2c8cc2250bd04)
on `codex/candidate03`. The clean public checkout built successfully, all five
native tests and all 13 packaged modes passed, and final rendered physicality
and four performance scenarios are recorded. Documentation and evidence can
follow that source commit without changing its packaged runtime identity.

The [Candidate03 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
provides the complete Windows archive and five review movies.
The immutable `candidate03` tag identifies the documentation/evidence revision;
its gameplay files are unchanged from the exact built source above.
[Native packaged input](../Evidence/Candidate03/StageE/native_input_review.md)
verified two-weapon access, firing/reloads, lighting, pause/death behavior and a
separate normal launch with sandbox/reset/restart controls. Sustained native
movement and perceptual sound approval remain outside the available evidence.
[Candidate02](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02)
remains the preserved playable rollback.

| Verified Candidate03 record | Result |
| --- | --- |
| [Fresh build](../Evidence/Candidate03/StageE/fresh_build.json) | Editor, Win64 game and Windows Development package; 569 tracked files and 291 current LFS payloads verified at S2 |
| [Native tests](../Evidence/Candidate03/StageE/fresh_build.json) | Five passed: two without warnings; three with five isolated test-world teardown warnings |
| [Packaged suite](../Evidence/Candidate03/StageE/packaged_checks.json) | 13 modes, 714 logged PASS assertions, zero FAIL; every mode exited zero with its own completion marker |
| [Rendered physicality](../Evidence/Candidate03/StageE/physicality_motion_review.md) | Separate final capture: 249 assertions, zero failures, 3,069 genuine JPEGs and bounded chronological review |
| [Performance](../Evidence/Candidate03/Performance/README.md) | Four completed captures: living 6/12/18 and physicality; the separate recording-free physicality run passed 257 assertions |
| [Archive](../Evidence/Candidate03/StageE/archive_audit.json) | `ProjectONE-Candidate03-Windows.zip`, 398,043,452 bytes, 1,217 verified entries |

Archive SHA-256:
`ada689b2314d3047d581d0523a2cbcd91f57661d06554d6288a6b4fae79600d2`.
The [release audit](../Evidence/Candidate03/StageE/release_audit.json) binds 45
retained runtime files to the fresh source build. The
[pass report](Passes/Candidate03.md) and [evidence index](../Evidence/Candidate03/README.md)
separate historical implementation runs, final automation, image inspection,
audio measurement and direct-input evidence.

## Implemented movement and weapons

WASD movement and mouse aim are independent. Player walk/sprint speeds are
225/370 cm/s, with eight authored directions for each gait, blended using
actual movement. Stationary aim changes use authored stepping turns and foot
preservation while the upper body continues aiming. Infected use 100 cm/s
shambling and 195 cm/s pursuit. These are editable gameplay values, not a claim
that every foot contact is visually perfect.

| Weapon | Ammunition | Fire | Reload |
| --- | --- | --- | --- |
| 5.56 mm Carbine | 24 loaded / 192 initial reserve | Automatic; 0.16-second cadence; 32 base damage | 2.10 seconds; magazine out 0.40, ammunition transfer 1.20, bolt 1.74 |
| 12-Gauge Pump Shotgun | 6 loaded / 36 initial reserve | One discharge per press; eight 15-damage pellets; 4-degree spread; 0.22-second recoil plus 0.56-second pump | 0.35-second opening; 0.90 seconds per shell with transfer at 0.60; 0.32-second closure |

Shotgun damage is full through 5 m and falls to 20% at 14 m. Carbine range is
28 m, with falloff after 14 m to 65% at maximum range. Later rounds add 48
reserve carbine rounds and eight shotgun shells, capped at 270 and 60.
Definitions expose ammunition, cadence, damage, trauma, operations and assets.

Held Left Shift has priority over manual R and automatic reload, even without
movement input. Pressing it cancels the active reload and permits sprint speed.
An empty equipped weapon with reserve automatically reloads after fire, pump
and equip obligations finish and sprint is released. Holstered weapons do not
reload automatically. Canceling retains only ammunition transferred by elapsed
events and clears unfinished events, temporary props and buffered fire.

Both slots retain ammunition and pending pump state when switched. Equip takes
0.36 seconds with the visible swap at 0.18. A short fire tap interrupts shotgun
shell loading through its closing pose; a spent shell ejects exactly once even
if the pump is interrupted by switching. Pause freezes operation time and
clears held/queued fire. Death cancels operations. Refill resets both weapons
to ready; restart reconstructs the encounter and inventory.

Authored ready, fire, pump, reload and equip actions layer over lower-body
locomotion. The fore-end, held shell and seated/held carbine magazine follow
those actions. A shaped muzzle flash and short light envelope attach to the
current evaluated gun pose; the thin world tracer remains separate. Six
original shot profiles per weapon avoid immediate repeats at fixed pitch.
Rifle brass ejects on discharge and shotgun hulls on pump extraction. Cases
inherit movement and spin, use swept flight with bounce/settle, and are capped
at 32 with a six-second lifetime. They do not block characters or navigation.

## Regional damage, remains and blood

A discharge traces all pellets before resolving one transaction per victim.
Each of six regions (body, head, anatomical left/right arm, left/right leg)
retains its own damage, trauma, pellet count and weighted impact position,
direction, normal and bone. Source `_r` maps to anatomical left after the
established import reflection; source `_l` maps to right. Runtime queries,
severed meshes and remaining-arm attacks use that same mapping.

All eligible sever decisions use the pre-shot presence state, so one blast can
detach multiple parts. Missing regions contribute no further damage. Head,
either arm and left leg are severable; right-leg hits damage and bleed but do
not detach it. Head loss, left-leg loss or losing both arms is fatal. Trauma
thresholds are 32 for head, 50 for either arm and 70 for left leg; arm health
damage is scaled by 0.4. Surviving arm loss selects the authored attack for the
remaining arm. Nonlethal hit reactions have a cooldown.

A per-victim cache rejects repeated nonzero IDs among the last 32 accepted
discharges. Health is applied once
per live transaction and the registered live set awards 100 points per kill
once. Fresh corpse hits can add finite bleeding, impulses and additional
eligible severing, without modifying health or awarding another kill.

Death and detached pieces capture the evaluated pose before beginning real
skeletal physics with constrained bodies, inherited velocity and bounded
impulses. Severed physics chains are terminated; left-leg removal transfers
stump coverage to a pose-fitted pelvis capsule. Remains collide with the room
and each other while ignoring character capsules and navigation.

The rest policy requires at least two seconds since disturbance, 1.25 seconds
of low motion across all active bodies, a stable measured pose and static
support before holding the evaluated pose as collidable kinematic bodies.
Fresh corpse damage/severing resumes retained bodies from that pose before
applying impulses. Contact alone leaves held remains fixed. This is a deliberate
stability/cost tradeoff, not continued simulation for every resting corpse.
The final packaged capture reports a fresh frozen-corpse hit resuming with
0.000000 cm / 0.038784-degree transition error. Its six-corpse phase ended with
32 awake bodies versus a peak of 96 and three frozen corpses; it does not show
that every body slept. The [rest review](../Evidence/Candidate03/StageE/physicality_rest_review.md)
separates those counters from the inspected images.

Wounds have finite visual blood budgets. Minor wounds emit briefly; severe
torso wounds and cuts emit more heavily. Gravity-driven droplets project onto
room surfaces; compatible deposits merge into gradually growing pools. Torso
injury is a surface wound, not a modeled opening. Caps are 96 wounds, 48 airborne
droplets, 90 decals, 14 corpses and 18 detached parts, with at most 12 surface
traces per frame. Corpse/part lifetimes are 28/18 seconds. Cleanup clears pending
sources, droplets, pools, decals and remains so old emitters cannot replenish
them afterward.

## Encounter and controls

Normal rounds begin after five seconds. Sandbox disables automatic waves and
uses the same combat, navigation and score registration, capped at 18 living
infected. Spawning tries bounded nearby positions on the player's floor and
requires a complete navigation path and capsule clearance. The existing room
layout is unchanged. Stale imported collision was replaced and saved navigation
rebuilt; the [before/after record](../Evidence/Candidate03/StageD/navigation_comparison.md)
separates that repair from the original failing crowd fixtures.

The HUD reads health, both weapon slots, ammunition, operation progress,
encounter counts, score and hit feedback from gameplay state.

| Input | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left Shift | Sprint; interrupt reload and defer further reload while held |
| Left mouse / R | Fire / manual reload |
| 1 / 2 | Select carbine / shotgun |
| Tab / either mouse-wheel direction | Cycle carried weapons |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or after death |
| F1 | Toggle sandbox and reset the encounter |
| F2 / F3 | In sandbox: spawn one / up to six registered infected |
| F4 | In sandbox: refill both weapons and reset their operations |
| F5 | In sandbox: reset the encounter |
| F6 | In sandbox: clear corpses, detached parts, cases and blood |
| F7 | In sandbox: toggle bright/dim room lights; dim uses 18% intensity with unchanged exposure |

## Verification and limits

The final 13-mode suite logged 714 PASS assertions: movement 58, weapon 85,
case 49, presentation 121, regional damage 42, physicality 245, legacy combat
40, comparison 3, legacy presentation 33, legacy validation 35, and one each
for the 6/12/18 benchmarks. Repeated timed assertions count as logged assertions,
not distinct test cases. All modes exited successfully. The record retains
208 warnings: 192 socket lookups without a mesh and 16 missing-Recast
crowd-manager messages. There were no Error, Fatal or ensure entries.
The five native tests separately retain five isolated test-world teardown
warnings across three successful tests.

Three physicality counts belong to different runs: the corrected editor-game
capture passed 244 assertions; the final S2 rendered capture passed 249; and
the separate recording-free profile passed 257. The default packaged suite's
physicality mode passed 245. Earlier failed settling captures and the failed
first-source packaged pursuit fixture remain preserved historical evidence;
they are not relabeled as S2 successes. Only the validation driver changed
between first source and S2. See the
[fixture chronology](../Evidence/Candidate03/StageE/first_source_validation.md).

The final S2 [motion](../Evidence/Candidate03/StageE/physicality_motion_review.md),
[blood/crowd](../Evidence/Candidate03/StageE/physicality_blood_crowd_review.md)
and [rest](../Evidence/Candidate03/StageE/physicality_rest_review.md) reviews
identify their inspected chronological windows. These support visible arm/head
separation, different corpse orientations, localized trails, exposed pool
growth, crowd approach through available aisles, and no obvious large resume
pose reset in the selected interval. They do not establish every hidden contact,
every actor's complete route or every frame of the recording.

Four final performance captures used UE 5.7.2 Windows Development at 1600×900,
VSync off and a 120 FPS cap on Ryzen 9 5950X / RTX 3090. Each living-crowd run
retains all 3,000 frames; full mean times for 6/12/18 were 8.3754/8.3818/8.3861 ms.
In the predeclared CSV-relative 10–20-second window, 18-enemy p99 rose from
Candidate02's 8.7933 to 9.0602 ms. Individual solver CPU costs increased; their
overlapping scopes cannot be added into a physics wall-time total. Each selected
living run includes its screenshot-associated frame above 100 ms.

The physicality profile recorded 14,038 frames over 117.0604 seconds, mean
8.3388 ms, p99 9.0456 ms and maximum 31.4258 ms. Five frames exceeded 16.7 ms;
none exceeded 33.3 ms. Media capture was disabled for that run, and transition
costs remain included. An existing unsaved Blender GUI stayed open under
explicit low-CPU preflight/accounting limits; Candidate02 had no equivalent
ambient sampling. These are one-run, capped comparisons with no confidence
interval or claim of uncapped capacity. Full scopes, spikes, settings and
limitations are in the [performance report](../Evidence/Candidate03/Performance/README.md).

The final packaged comparison's genuine 20.992-second engine WAV covers both
weapon phases with zero full-scale samples and measured peak -13.889215 dBFS.
The capture runner supplies process-only background unmute; normal game audio
configuration is unchanged. Numerical energy and variation checks do not
establish timbre or mix quality. Perceptual listening/approval remains unavailable.

Characters remain simplified: compressed knees and low strides look mechanical,
small reload props/cases can be hard to read, and close crowds hide individual
feet. Some corpse poses remain awkwardly propped with crossing limbs; pools
are partly hidden and their edges remain stamp-like. Room-corner views expose
black off-map space. Right-leg removal is not implemented. Supported frozen
corpses do not react to ordinary contact until fresh damage/severing resumes
them. Available native automation did not deliver sustained W movement or
held Shift edges; those behaviors were tested through frame-spaced production
input dispatch, without a claim of sustained native keyboard playtesting.
Visual samples, numerical assertions and direct input remain separate evidence.

## Reproduce the verified source

Use Git LFS and explicitly check out
`e81e1137ed891570c73c8c278fc2c8cc2250bd04` before building; later documentation
commits are not the package source. The [README build instructions](../README.md#play-or-build)
include the exact checkout, LFS verification and build/package/validation commands.
Prerequisites are Unreal Engine 5.7.2, Visual Studio C++ build tools and a
compatible Windows SDK. The recorded toolchain used MSVC 14.50.35725 and SDK
10.0.26100.0; Unreal reported the compiler as newer than its preferred 14.44.
Blender 5.1.2 is only needed to author/regenerate art. Players launch the complete
extracted archive's root `ProjectONE.exe`; the archive includes Windows
prerequisites and does not require the editor or Blender.

## Preserved baseline and source references

The preserved prior playable release is
[Candidate02](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02).
Its source baseline is `aa4d55d04bf375bbef41362af77eec10d9ea224f`; packaged gameplay
revision is `0637288d32b6fbebc67ef93d4f03e439ff38bb67`. The
[Candidate02 pass report](Passes/Candidate02.md) and
[evidence](../Evidence/Candidate02) retain its exact checksums and verified
results. Candidate01's public baseline is
`02c590a75141b48564715d130018f0cbe9a8a092`; previous candidates are preserved.
Project Zero remains excluded. No broad license has been chosen and no paid
storage settings were enabled. Required original source/assets use Git LFS;
normal uploads/fetches were verified, while account-specific quota/budget
information was unavailable under the current authorization.

Implementation entry points are [weapon definitions](../Source/ProjectONE/ONEWeaponTypes.h),
[weapon operations](../Source/ProjectONE/ONEWeaponComponent.cpp),
[animation](../Source/ProjectONE/ONEAnimInstance.cpp),
[infected damage](../Source/ProjectONE/ONEZombie.cpp),
[physical remains](../Source/ProjectONE/ONEPhysicsRuntime.cpp) and
[blood scheduling](../Source/ProjectONE/ONEBloodSubsystem.cpp).
[Character sources](../ArtSource/Characters/Candidate03/README.md),
[weapon presentation inventory](../ArtSource/Weapons/Candidate03/presentation_inventory.json),
[audio sources](../ArtSource/Audio/Candidate03/README.md),
[provenance](Provenance.md) and the [pass record](Passes/Candidate03.md)
preserve authoring and verification context. Further weapons, archetypes,
progression and map expansion are not implemented by this pass.
