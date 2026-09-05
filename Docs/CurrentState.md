# Current state — Candidate03 development

Candidate03 is the correction pass on `codex/candidate03`, continuing the
existing containment arena, two carried weapons, one infected archetype and
round/sandbox modes. Internal stages B and C are complete. Stage D passed
244 rendered integration checks and focused freeze/resume review. Stage E's fresh-checkout build,
combined packaged checks and publication are pending. **No Candidate03 public
release exists yet.** The [Candidate03 pass record](Passes/Candidate03.md) owns
the detailed stage evidence and final publication fields.

| Candidate03 publication field | Current status |
| --- | --- |
| Exact packaged source revision | Pending final validation and fresh build |
| Fresh-checkout build and LFS/source-hash audit | Pending |
| Final packaged suite and extracted-archive review | Pending |
| Release URL, archive size and SHA-256 | Pending publication |

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

Replayed discharge IDs do not repeat damage or severing. Health is applied once
per live transaction and the registered live set awards 100 points per kill
once. Fresh corpse hits can add finite bleeding, impulses and additional
eligible severing, without modifying health or awarding another kill.

Death and detached pieces capture the evaluated pose before beginning real
skeletal physics with constrained bodies, inherited velocity and bounded
impulses. Severed physics chains are terminated; left-leg removal transfers
stump coverage to a pose-fitted pelvis capsule. Remains collide with the room
and each other while ignoring character capsules and navigation.

The implemented rest policy requires a supported, stable physical pose before
holding it as collidable kinematic bodies. Fresh corpse damage/severing resumes
the retained bodies from that pose before applying impulses. Contact alone
leaves held remains fixed. This deliberate cost/stability limit and its
freeze/resume continuity are still undergoing final rendered validation.

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

Stage B's internal evidence includes 48 directional speed trials, 85 weapon
checks including production input bindings, and rendered directional/turn
review. Stage C passed 121 presentation assertions and 49 independent case
checks with genuine rendered sequence review. These editor-game stage results
are not the final Candidate03 packaged result.

Stage D's regional damage, physics, wound and navigation integration has been
exercised internally. Earlier full captures failed corpse settling and are
retained as failures. The final freeze/resume correction and rendered review
are underway. Final native tests, all 13 default packaged modes, recording-free
CSV performance comparison, source/asset hashes, fresh build and extracted
archive review remain part of stage E; no final pass or hardware-performance
claim is made here.

Characters remain simplified; knee compression, cadence and small reload-prop
readability remain provisional. Audio is original local synthesis with measured
variation and genuine engine recordings, but perceptual listening/approval is
unavailable. Available native automation did not deliver held Shift key edges;
that behavior was checked through frame-spaced production input dispatch, not
claimed as a sustained human playthrough. Visual samples, automated assertions,
recordings and source inspection are separate forms of evidence.

## Preserved baseline and source references

The published playable build remains
[Candidate02](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02).
Its source baseline is `aa4d55d04bf375bbef41362af77eec10d9ea224f`; packaged gameplay
revision is `0637288d32b6fbebc67ef93d4f03e439ff38bb67`. The
[Candidate02 pass report](Passes/Candidate02.md) and
[evidence](../Evidence/Candidate02) retain its exact checksums and verified
results. Candidate01's public baseline is
`02c590a75141b48564715d130018f0cbe9a8a092`; previous candidates are preserved.
Project Zero remains excluded. No broad license has been chosen.

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
