# Current state — Candidate02

Candidate02 is the original Project ONE containment arena with two carried
weapons and a developer sandbox. It retains the accepted player, infected
archetype, room, rounds, points, blood and severing foundation. Project Zero
content remains excluded.

The public Candidate01 baseline is
`02c590a75141b48564715d130018f0cbe9a8a092`. Public release references are `main`
for latest, `codex/candidate02` for review and the `candidate02` version tag.
The original Candidate01 history bundle, local rollback tag and package remain
preserved separately. Packaged source revision:
**`0637288d32b6fbebc67ef93d4f03e439ff38bb67`**.
The [pass report](Passes/Candidate02.md) owns the final archive checksum, size and
verification record.
The final repository commit adds documentation, evidence and
release/validation/capture tooling; `Source`, `Config` and `Content` remain
identical to that packaged revision.
Required source and assets are published through Git LFS, with ordinary upload
and fetch verified. No broad license or paid settings were enabled;
account-specific LFS quota and budget were unavailable under current authentication.

## Implemented behavior

| Weapon | Ammunition | Fire | Reload |
| --- | --- | --- | --- |
| AR-01 carbine | 24 loaded / 192 initial reserve | Automatic, 0.16-second cadence, 32 base damage | 2.10 seconds; magazine out 0.40, ammunition transfer 1.20, bolt 1.74 |
| SG-01 pump shotgun | 6 loaded / 36 initial reserve | One discharge per press; eight 15-damage pellets, 4-degree spread; 0.22-second recoil plus 0.56-second pump | 0.35-second opening, 0.90 seconds per shell with transfer at 0.60, 0.32-second closure |

Shotgun damage is full through 5 m and falls to 20% at 14 m. Carbine range is
28 m with falloff after 14 m. Definitions expose capacities, reserves, cadence,
falloff, trauma tuning, operations and animation/audio references. Later rounds
add 48 reserve carbine rounds and eight shotgun shells, capped at 270 and 60.

Both slots retain ammunition and pending pump obligations when switched. Equip
takes 0.36 seconds with the visible swap at 0.18. Canceling a reload retains only
earned ammunition. A short fire tap interrupts shell loading through its closing
pose; an interrupted pump ejects its spent case only once. Pause freezes operation
time and clears queued fire. Death cancels operations; refill resets both weapons
to ready; restart reconstructs the player, encounter and ammunition.

The native animation graph layers ready, fire, pump, reload and equip clips over
directional leg locomotion. A separate moving fore-end, held shell and seated/held
carbine magazine follow the authored actions. Five new static meshes, nine clips
and 25 original synthesized event sounds retain editable sources alongside the
accepted rigs and meshes. No broad character or environment redraw was performed.

Pellets aggregate into one damage transaction per victim/discharge; duplicate
packets and corpse hits cannot duplicate damage, severing or points. A blast can
detach at most one part per victim, with lethal head loss taking priority.
Surviving arm loss retains the remaining-arm attack. Significant nonlethal blasts
can trigger a cooldown-limited heavier reaction. Registered kills award 100 points
once. Blood, bodies, detached parts and spent cases remain bounded; detached
meshes preserve the evaluated pose and do not block movement.

The HUD reads health, both weapon slots, ammunition, operation progress, encounter
counts, score and hit/kill feedback from gameplay state. Normal rounds remain
available. Sandbox disables automatic waves, keeps the same combat/registration
systems, caps living infected at 18 and adds 0/2/5/10 m references to the arena.
Sandbox spawning now tries up to 17 bounded nearby positions on the player's
floor and requires a complete navigation route plus capsule clearance before
acceptance. It does not relocate the actor after those checks.

## Controls

| Input | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left Shift | Run; reload limits movement to walking speed |
| Left mouse / R | Fire / reload |
| 1 / 2 | Select carbine / shotgun |
| Tab / either mouse-wheel direction | Cycle carried weapons |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or after death |
| F1 | Toggle sandbox and reset the encounter |
| F2 / F3 | In sandbox: spawn one / up to six registered infected |
| F4 / F5 / F6 | In sandbox: refill both weapons / reset encounter / clear remains and blood |

## Verification and open limits

The separate verified public clone was cleanly fast-forwarded to `0637288`;
editor/game/package builds passed, with 202 hydrated LFS files totaling
78,275,144 bytes and unchanged tracked source on the existing validation host.
All 78 imported source hashes match. The retained native test report passed three
tests. All seven final packaged modes passed: 39 weapon checks, retained-system,
presentation and comparison checks, plus performance runs with 6/12/18 live
infected. Mean frame times were 8.385/8.387/8.388 ms, all median/p95 8.334 ms
at 1600×900 on an RTX 3090 under a 120 FPS cap. These
capped measurements do not establish uncapped headroom or lower-end performance.

[Direct manual input review](../Evidence/Candidate02/manual_playtest.md) found
two sandbox-spawned infected stranded on rack
navigation islands. The corrected source adds the bounded route/clearance search
above and a regression using the F2/F3/F3 sequence with 13 enemies and an
eight-second approach. All 13 accepted enemies had complete routes, stayed on
the floor and approached in the final packaged regression. Direct manual F2
follow-up also confirmed a floor spawn reaching and attacking the player. That
review found inherited F2/F5 debug commands changing render modes; final input
configuration removes five conflicting F1–F5 bindings. Final F1–F6 manual review:
**passed on `0637288d32b6fbebc67ef93d4f03e439ff38bb67`**. Normal lighting remained
intact, floor spawns approached, and weapon fire/reload/pause/switch,
refill/reset and quit checks passed in that short review. Exact cleanup counts
remain covered by automation. This was not a sustained human playthrough or
clean-machine installation test.

The final [extracted-archive startup check](../Evidence/Candidate02/release_smoke.json)
verified all manifest hashes/sizes and rendered the arena and ordinary round.
A Windows firewall prompt then limited that smoke test; no security-prompt action
was taken. Full controls had already passed on the identical adopted package.

The final comparison video contains 470 genuine captured frames at 22.38638 FPS
with 20.97066667 seconds of engine audio. Its 13-frame visual review found coherent
pump hand/fore-end positions and changing leg poses, while small shells and dark
magazine surfaces limit contact readability. Those stationary reload samples do
not establish moving-reload quality. Stills, automated pose measurements and
direct manual play are separate evidence types; none substitutes for the others.

Audio measurements show complete, non-silent weapon phases, −13.019964 dBFS sample
peak, −37.584901 dBFS RMS and zero full-scale samples. An earlier silent background
capture was rejected; the accepted recording uses a process-only background
unmute override without changing packaged configuration. Perceptual audition was
unavailable. The sounds are original local synthesis; realistic timbre and mix acceptance remain provisional. The accepted
characters are still visually simplified, and the inherited source-Y reflection
and anatomical labels remain consistently handled by runtime bindings. No final
art or audio approval is claimed. The visual review records the final `0637288`
build and its exact inspected timestamps; it is not evidence about every frame.

## Continue from here

The [pass report](Passes/Candidate02.md) links the evidence, toolchain and release
record. [Weapon definitions](../Source/ProjectONE/ONEWeaponTypes.h),
[operation/damage dispatch](../Source/ProjectONE/ONEWeaponComponent.cpp),
[animation blending](../Source/ProjectONE/ONEAnimInstance.cpp) and
[game mode](../Source/ProjectONE/ONEGameMode.cpp) are the principal implementation
entry points. The [asset inventory](../ArtSource/Weapons/Candidate02/README.md),
[provenance](Provenance.md), [character pipeline](CharacterPipeline.md) and
[environment pipeline](EnvironmentPipeline.md) preserve authoring context.

The next focused recommendation is human listening
and gameplay-camera weapon-feedback review, especially reload prop readability.
Further weapons, enemy types, progression and map expansion are not implemented
or scheduled by this record.
