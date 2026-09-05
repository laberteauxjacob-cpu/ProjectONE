# Candidate04 — weapon progression and physical machines

Candidate04 adds a starter pistol, owned weapon slots, a Mystery Box and a
Pack-a-Punch upgrade process to the existing containment arena. The three base
weapons now have distinct assemblies and one upgrade each. The existing round
mode, sandbox, infected archetype, regional damage, finite bleeding and physical
death behavior continue from [Candidate03](Candidate03.md). Candidate03 remains
the preserved playable baseline.

This report describes the implementation and completed editor checks. Final
source revision, fresh-checkout build, packaged verification, performance result,
archive and public Candidate04 release are pending. Capture review is also in
progress. A passing functional check does not establish visual approval,
perceptual sound quality or ordinary held-key playtesting.

## Owned inventory and machine behavior

A fresh run starts with an M1911 in slot 1, seven loaded rounds and 56 reserve,
an Empty slot 2, and zero points. The six effective weapon definitions are a
catalog; the player still owns at most two weapon instances. Empty, available,
machine-reserved and ready-to-collect are separate slot states. Each owned
weapon retains its ammunition and mechanical state when switched. Depositing
the only weapon leaves the player visibly unarmed but able to move and use the
other machine.

Holding F for 0.4 seconds operates the offer shown at a reachable machine front.
The offer includes the relevant weapon, slot and cost. Releasing early, leaving
reach, losing line of sight, pausing, or changing the relevant inventory state
cancels an unfinished hold. Completion and cancellation both require a release
before another hold can act. Keeping F down through a machine cycle cannot
collect the result or purchase again.

The Mystery Box charges 950 points when a purchase succeeds. It selects one
eligible base family, cycles for five seconds and then waits indefinitely for
collection. Default family weights are equal; weights and cycle duration are
editable. A family reserved at Pack-a-Punch is excluded. Collection refills an
already-owned family while preserving its upgraded instance; otherwise it fills
an Empty slot or replaces the currently selected available weapon. The prompt
discloses that decision. A duplicate already at full supply can be consumed
explicitly without adding ammunition. A reserved slot never counts as Empty.

Pack-a-Punch charges 5,000 points and accepts a ready base weapon with its
magazine present and no outstanding pump operation. The short handoff reserves
and charges at its 0.48-second acceptance event. Processing then takes nine
seconds. Movement, firing, reload and switching are locked during the handoff
and retrieval animations; movement resumes during processing, and another
available weapon remains usable. A box purchase can fill the other Empty slot
while the first weapon is reserved.

Ready output does not expire. A fresh F hold at the output starts retrieval;
the take event occurs 0.18 seconds into its 0.64-second action. Collection returns
the same weapon instance to the same original slot, upgrades it once, and fills
its effective capacity and maximum reserve once. The normal equip operation
installs the carried assembly. The machine closes and resets in 0.85 seconds.

Leaving before deposit acceptance cancels without payment. Walking away after
acceptance preserves processing and the reservation. Leaving contact before
the retrieval take event restores the ready output for a later attempt.
Technical machine failure or destruction before delivery restores the original
weapon snapshot and refunds the paid receipt at most once in the current living
run. Death and restart invalidate the old machine transactions. A delayed or
repeated callback cannot deliver a weapon, restore an old instance or refund
points into the next run. Registered infected kills continue to award 100 points
once through the authoritative live-enemy set.

## Controls

| Input | Behavior |
| --- | --- |
| WASD / mouse | Independent movement / aim. Player walk and sprint remain 225 / 370 cm/s. |
| Left Shift | Sprint and interrupt reload; held sprint prevents manual and automatic reload, including while stationary. |
| LMB | Fire; M4A1 and Overcurrent are automatic, pistols and pump shotguns fire once per press. |
| R | Reload; recover a canceled pistol/rifle reload whose magazine has already been removed. |
| 1 / 2 | Select the corresponding available owned slot. Empty and reserved slots cannot equip. |
| Tab / either mouse-wheel direction | Cycle available slots. |
| Hold F | Buy, deposit or collect the displayed machine offer; release between actions. |
| Escape | Pause/resume. |
| Enter / Q | Restart / quit while paused or dead. |
| F1 | Toggle sandbox and restart the arena. |
| F2 / F3 | Sandbox: request one / six reachable infected, within the active cap. |
| F4 / F5 | Sandbox: refill available weapons to their initial supplies / restart sandbox. |
| F6 / F7 | Sandbox: clear blood/gore presentation and cases/magazines / toggle bright/dim room lighting. |
| T | Sandbox: add 10,000 points, recorded separately from earned points. |
| Z / X / C | Sandbox: force the next eligible box reward to M1911 / M4A1 / Remington 870. |
| V | Sandbox: reset the next box reward to random. |

A forced reward whose family is reserved is disabled with an explicit prompt to
reset it to random. A successful roll consumes the forced selection. Existing
F1–F5 engine rendering shortcuts remain removed so they do not conflict with
the sandbox controls.

## Effective weapon balance

These are editable defaults. Damage is close-range raw body damage before
regional handling and falloff; pellet placement affects actual damage. Listed
intervals are configured cadence, not measured sustained damage per second.

| Weapon | Fire mode | Damage × pellets | Capacity | Initial / maximum reserve | Fire interval |
| --- | --- | --- | ---: | ---: | ---: |
| M1911 | Semi | 28 × 1 | 7 | 56 / 84 | 0.240000 s |
| Last Word | Semi | 56 × 1 | 14 | 112 / 168 | 0.208696 s |
| M4A1 | Automatic | 32 × 1 | 24 | 192 / 270 | 0.160000 s |
| Overcurrent | Automatic | 64 × 1 | 36 | 384 / 540 | 0.139130 s |
| Remington 870 | Pump/semi | 15 × 8 | 6 | 36 / 60 | 0.780000 s |
| Gravebreaker | Pump/semi | 30 × 8 | 8 | 72 / 120 | 0.678261 s |

Each upgraded definition independently doubles damage and reserve limits,
increases capacity as shown, and raises configured firing rate by 15 percent.
The interval is divided by 1.15. Fire and pump animation durations, fore-end
motion and mechanical events use the same scaling; reload and equip timings
remain unchanged. Base definitions and future box rewards are not mutated by
upgrading an owned instance. Overcurrent narrows spread from 0.35 to 0.25
degrees. Gravebreaker lowers the heavy-stagger threshold from 70 to 60.

Pistol range is 24 metres, with falloff starting at 10 metres and a 55-percent
minimum. Carbine values are 28 metres, 14 metres and 65 percent; shotgun values
are 14 metres, five metres and 20 percent. Round transitions after round 1 grant
available pistol/carbine/shotgun slots 14/48/8 reserve respectively, bounded by
their effective maximum. Reserved weapons are excluded.

Last Word alone can penetrate one additional victim at 60-percent damage, with
normal falloff still measured from the original muzzle. World cover stops the
continuation and the original range endpoint never extends. The entire previous
victim is ignored, preventing repeated hits on its other region shapes. The
existing six-region packet aggregation resolves each victim once per discharge;
third-victim chaining is excluded.

## Reloads, physical presentation and audio

An empty equipped weapon reloads automatically only when it has reserve and
becomes eligible after any required pump. Sprint, pause, death and handoff locks
prevent it. Canceling reload retains ammunition earned by elapsed events and
clears future events and obsolete props. Holstered weapons do not auto-reload.
A short shotgun trigger press can close a per-shell reload and retain one
buffered shot; explicit input cleanup clears that buffer.

Pistol reload lasts 1.8 seconds: the old magazine releases at 0.28 seconds, the
fresh held magazine appears at 0.64, ammunition commits at 1.10, and the slide
releases at 1.40. Rifle reload lasts 2.1 seconds, with corresponding release,
fresh-prop, commit and bolt events at 0.40, 0.74, 1.20 and 1.74 seconds. The old
magazine releases from the actual seated component's current world pose. An
interrupted and restarted reload cannot drop it twice. If a loaded reload was
canceled after removal, firing cannot bypass the missing magazine; R explicitly
reinstalls it. Recovery with no reserve does not invent ammunition.

Base shotgun fire lasts 0.22 seconds followed by a 0.56-second pump. Ejection
occurs 0.18 seconds into pumping and lock occurs at 0.44 seconds. Per-shell
reload retains its 0.35-second start, 0.90-second insert with ammunition earned
at 0.60, and 0.32-second end. Pistol/rifle cases eject on discharge, while shotgun
cases retain their pump-event identity across interruption and resumption.

Cases are bounded to 32 actors for six seconds; old magazines to 12 for eight
seconds. They inherit movement, follow gravity, sweep supporting collision,
bounce and settle without blocking characters, shots or navigation. They are
cosmetic debris, not ammunition pickups. Their bounded custom flight solver is
separate from the retained Chaos infected and detached-part physics.

The new weapon assemblies include moving pistol slides, separate magazines and
shotgun fore-ends. Upgraded assemblies use violet, cyan and ember accents with
corresponding tracer and muzzle-light colors. Each effective weapon has six
shot variations selected without immediate repetition at pitch 1.0. Candidate04
adds 28 original mono 48 kHz PCM16 files: 24 shots for the M1911 and three upgrades,
plus four pistol mechanical/empty cues. Base carbine/shotgun banks are retained
from Candidate03. Reproducible generation, distinct files, peaks and non-clipping
samples have been checked; these measurements do not establish perceptual audio
quality.

## Verification checkpoint and remaining work

| Checkpoint | Completed result | Remaining verification |
| --- | --- | --- |
| Editor Arsenal integration | 148 assertions passed, zero failures after target-pose, exact shotgun event-window and near-expiry checks. | Final packaged run pending. |
| Editor Progression integration | 72 assertions passed, zero failures, including real nine-second processing, level restart, pre-take retrieval cancellation and output restoration. | Final packaged run pending. |
| Initial native suite | Eight of nine tests passed. | The sole failure was the firing-rate comparison's float tolerance. Its corrected targeted inventory suite passed all four tests with zero warnings; a final complete suite remains pending. |
| Presentation capture 01 | All 58 assertions passed using scripted production input, targeting 30 JPEGs/s with the game capped at 120 fps. | Functional capture success is separate from visible quality. Capture 02 also passed 58 checks at a 30 fps game cap, producing 2,890 genuine frames and 148.331 seconds of engine WAV. Final packaged recording remains pending. |
| Fresh source, package and release | Pending. | Exact source revision, clean-checkout build, packaged checks, performance, final media review, archive audit and publication. |

The legacy weapon test initially failed because its test loadout was installed before starter initialization. The test-only setup now runs on the next world tick; the unchanged weapon assertions pass all 85 checks. The earlier movement result is not accepted as two-family coverage. New identity assertions require the correct slot and family before each of its 54 trials; the corrected full movement suite remains pending in the fresh package. The failed run is retained separately.

Arsenal uses a declared collision floor outside the arena and unregistered
high-health torso targets. Actual catalog damage, spread, range and mechanical
timings are unchanged. Owned-instance setup bypasses machine waiting only for
isolated weapon comparisons. Its debris-cap fixture extends emitted magazine
lifetimes to isolate eviction, while a separate check measures production
expiry. These fixtures do not establish navigation, scoring, art or performance.

Progression dispatches frame-spaced keys through the production player
controller, uses declared approach and point fixtures, and exercises actual
machine waits and level restart. That covers production bindings and transaction
behavior, not sustained ordinary OS keyboard input. Final moving presentation,
machine contact, lighting readability and audio review remain distinct work.
No Candidate04 release or final source/package identity is claimed at this
checkpoint.

The implementation is concentrated in
[`ONEWeaponCatalog.cpp`](../../Source/ProjectONE/ONEWeaponCatalog.cpp),
[`ONEWeaponInventory.cpp`](../../Source/ProjectONE/ONEWeaponInventory.cpp),
[`ONEWeaponComponent.cpp`](../../Source/ProjectONE/ONEWeaponComponent.cpp),
[`ONEWeaponMagazine.cpp`](../../Source/ProjectONE/ONEWeaponMagazine.cpp),
[`ONEInteractionComponent.cpp`](../../Source/ProjectONE/ONEInteractionComponent.cpp)
and [`ONEProgressionMachine.cpp`](../../Source/ProjectONE/ONEProgressionMachine.cpp).
Player, animation, HUD and machine-presentation integration use those same
definitions and instance states. Original weapon and machine generators,
importers and editable assets remain under `Scripts` and `ArtSource`; the three
new test sources are `ONE04WeaponTests`, `ONE04ProgressionCheck` and
`ONE04ArsenalCheck` in `Source/ProjectONE`.
