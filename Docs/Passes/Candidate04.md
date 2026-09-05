# Candidate04 — weapon progression and physical machines

Candidate04 adds a starter pistol, owned weapon slots, a Mystery Box and a
Pack-a-Punch upgrade process to the existing containment arena. The three base
weapons now have distinct assemblies and one upgrade each. The existing round
mode, sandbox, infected archetype, regional damage, finite bleeding and physical
death behavior continue from [Candidate03](Candidate03.md). Candidate03 remains
the preserved playable baseline.

The verified gameplay source is
[`8055041ebc98a4df7cd8923b05e7b89ad7372e38`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/8055041ebc98a4df7cd8923b05e7b89ad7372e38).
It built from a clean public checkout, passed all nine native tests, and passed
all 15 packaged modes with 984 assertions and zero failures. Final packaged
recording, bounded frame review, audio measurement and archive audit are complete.
Native launch was observed, but a security prompt blocked gameplay input. Both
movies and three media-free profiles are verified; publication checks are
recorded separately after upload. Functional
checks do not establish visual approval, perceptual sound quality or ordinary
held-key playtesting.

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

The original machine assemblies make the process visible. The containment chest
has sliding locks, a separately hinged lid, moving hydraulic struts and an
illuminated rotor. The processor feeds the actual assembled weapon inward on a
cradle, closes opposing clamps and chamber shields, rotates its field rings and
returns the upgraded weapon to the output. Preview parts retain authored scale;
handoff starts from the evaluated carried pose and retrieval follows the moving
hand. State lights and bounded original mechanical audio accompany the cycle.
Decorative parts are separate from explicit player/navigation collision shapes.

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

## Verification and limits

| Check | Verified result | Evidence |
| --- | --- | --- |
| Public source build | UE 5.7.2 Editor, Win64 game and Development package built successfully; 875 tracked files and 488 LFS payloads totaling 233,654,563 bytes verified. | [Fresh build](../../Evidence/Candidate04/Verification/fresh_build.json) |
| Native tests | 9/9 passed: six clean, three with five isolated test-world teardown warnings. All four inventory tests are warning-free. | [Exact test names and warnings](../../Evidence/Candidate04/Verification/fresh_build.json) |
| Packaged functional suite | 15/15 modes; 984 assertions, zero failures, all processes exited with code 0. Six required runtime hashes match the exact source build. | [Suite and fixture coverage](../../Evidence/Candidate04/Verification/packaged_suite.json) |
| C04 progression and arsenal | 72 progression and 148 arsenal checks passed in both final editor and packaged runs. Arsenal verifies all six effective variants; progression includes actual machine waits, retrieval cancellation, recovery and level restart. | [Packaged counts](../../Evidence/Candidate04/Verification/packaged_suite.json) |
| Source privacy | Earlier full working-payload audit plus final 318-file text audit, with two public source commits accounted for. No post-commit full binary rescan is claimed. | [Scope and limitations](../../Evidence/Candidate04/Verification/source_privacy.json) |

The initial native suite passed eight of nine tests; its only failure was a
float-tolerance assertion for the 15-percent firing-rate increase. The corrected
targeted inventory suite passed 4/4 without warnings, followed by the complete
fresh-build 9/9 result. The five remaining native warnings are
`UWorld::DestroyActor: World has no context!` during isolated test-world teardown.

The legacy weapon fixture initially installed its loadout before ordinary
starter initialization, which replaced it with the new pistol start. The fixture
now installs base M4A1/870 on the next world tick; all 85 unchanged weapon checks
pass in the final package. The earlier movement result is excluded as invalid
two-family coverage. Its corrected packaged run passes 112 assertions, including
the requested slot and family before all 54 trials. The failed runs are retained
separately; the corrections do not change normal starter inventory.

The checkout came from the public remote and was clean before verification.
Its final build fast-forwarded from the initial C04 source, so timings include
existing build caches. This is reproducibility on the validation host, not a
clean Windows installation without development prerequisites. The build record
retains its original packaged-suite-pending checkpoint; the later suite report
supplies the completed runtime result.

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
machine contact, lighting readability and audio quality are assessed separately.

The final packaged walkthrough passed 58 checks, recording 3,001 original frames
and 148.11733333 seconds of genuine engine master audio. The first packaged
attempt also passed 58 checks, with 2,932 frames, but strict assembly rejected
its final callback 34.454 ms beyond the WAV. The second capture's final callback
exceeded its WAV by 9.795667 ms. The explicit
[terminal-idle assembly policy](../../Evidence/Candidate04/Tools/README.md)
preserves all 3,001 raw frame identities and omits only `frame_03000.jpg` from
presentation. It retains 3,000 callbacks and 2.956479 seconds of the final idle
chapter, keeping the entire original audio without padding or stretching.
No firing, reload, transfer or machine transition is removed.

[Visual inspection](../../Evidence/Candidate04/Verification/visual_review.json)
covered 75 original frames across both captures: 68 in dense chronological
windows and seven sparse endpoints. Handoff, retrieval, the machine forms and
case flight are legible; fixed fingers, tiny shell seating, slide motion and
exact fore-end contact remain unresolved at gameplay scale. Intermittent missing
HUD glyphs persist in the original JPEGs. Native overviews with intact text do
not prove this is limited to capture. This is bounded frame inspection, not
full-movie playback or unrestricted visual approval.

The [engine audio audit](../../Evidence/Candidate04/Verification/audio_metrics.json)
measured all 23 selected phases with signal and no warnings. Overall peak was
-14.632356 dBFS and RMS -41.983174 dBFS, with no full-scale samples. These are
waveform measurements; no perceptual audition, timbre or mix approval occurred.

The [full progression movie](../../Evidence/Candidate04/progression_loop.mp4)
runs 148.117333 seconds with 80 verified chapter titles. The
[comparison](../../Evidence/Candidate04/weapon_comparison.mp4) runs 67.9 seconds,
with eight chapters and 2,037 video frames, selecting base/upgraded pairs and
dim-machine excerpts. Both complete audio/video decodes passed. Their
[full](../../Evidence/Candidate04/progression_loop.json) and
[comparison](../../Evidence/Candidate04/weapon_comparison.json) sidecars bind
the original/presented capture, actual encoded source and exact audio intervals.
The concise cut changes chronology through disclosed hard cuts; it is not
continuous gameplay.

The public evidence tools adapt the terminal-idle policy separately from the
built game. A stream-copy remux repaired missing container chapter titles in
both movies; encoded audio/video packet hashes remained identical, and chapter
titles/timing were verified without the missing-track warning. The originals
remain preserved. This changed evidence metadata only; the runtime was not
rebuilt or modified after S2. Encoding and successful decoding do not constitute
visual playback review or audio audition.

Three completed [media-free profiles](../../Evidence/Candidate04/Performance/README.md)
used 1600x900, cap 120 and VSync off, each passing 35 scenario checks. They
exercise Last Word fire/reload, movement, replenished living targets, real
corpses and both operating machines. Each includes 70 committed Last Word
shots and about 3.70 seconds with both machines Active. The machine processes
M4A1 to Overcurrent; carried Overcurrent and Gravebreaker are not profiled.

| Requested live target | Full numeric frames | Exact target in valid counter samples | Mean / p95 / p99 / maximum frame ms | Frames >16.7 / >33.3 ms |
| --- | ---: | ---: | --- | --- |
| 6 | 3,766 | 75.01% | 8.3350 / 8.8103 / 9.4036 / 12.5948 | 0 / 0 |
| 12 | 3,766 | 71.42% | 8.3369 / 8.8281 / 9.3608 / 17.1668 | 1 / 0 |
| 18 | 3,733 | 78.40% | 8.4123 / 9.1053 / 9.7091 / 22.0846 | 1 / 0 |

All numeric rows and spikes remain in the reports. Initial machine construction,
acquisition and pistol-upgrade setup precede CSV; setup may leave a corpse or
blood. Actual living counts vary during combat, so the targets are not constant
loads. The 10–20-second and simultaneous-machine subsets reference exact ranges
in each full timeline. MachineState mean CPU scope is 0.0268–0.0293 ms;
MachineVisual is nested within it and must not be added as unique CPU time.
Physics scopes likewise overlap.

Candidate03's stationary living-only reference includes a screenshot spike;
Candidate04 includes moving combat and machines without recording. The
[comparison](../../Evidence/Candidate04/Performance/comparison.json) shows full
p99 increases of 0.5883–0.7059 ms, but these different workloads, one run per
count and background/power state prevent causal or GPU-cost claims. The existing
idle Blender allowance includes boundary CPU accounting, which does not exclude
short bursts or establish identical ambient conditions.

The [ordinary native attempt](../../Evidence/Candidate04/native_controls.json)
used `Launch.ps1 -Candidate Candidate04 -Sandbox` on the separately extracted,
hash-verified archive. The launcher exited with code 0, and a native window
capture showed M1911 7/56, Empty slot 2, health 100 and zero points. A Windows
Security firewall prompt overlaid the game and blocked gameplay input. The
prompt was not acted on; user dismissal was requested but not received. No
native keyboard/mouse gameplay event or video was produced. Only the two
identity-checked test game processes were stopped afterward, which is process
cleanup rather than a successful in-game Quit test. Sustained ordinary movement,
sprint and Hold F remain unverified.

The complete `ProjectONE-Candidate04-Windows.zip` was audited and separately
extracted without replacing Candidate03. It contains 1,217 entries and
407,635,789 bytes; SHA-256 is
`1ad1a3258f16fccf12451fff829aa9124321491a457232b88efa871826ba783c`.
The [archive audit](../../Evidence/Candidate04/Verification/archive.json)
verifies every manifest size/hash and CRC, all 45 runtime files against the
fresh package, all six source-bound required runtime files, and 1,169 retained
notices. Its integrity result does not claim publication or new runtime testing.

The
[candidate04 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate04)
carries the Windows archive and evidence. Its separate
[Candidate04-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate04/Candidate04-PublicationVerification.json)
attachment records verification performed after upload, including exact final
commit, downloaded hashes, full outgoing history audit and hydrated public
checkout/LFS. Source is on
[codex/candidate04](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/codex/candidate04);
the [candidate04 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate04)
and [comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate03...candidate04)
identify the release review. Built gameplay source and later documentation/evidence
publication revision are distinct; this report does not embed its own commit.
The earlier source-privacy report retains its historical audit scopes; the
post-publication attachment records the final outgoing audit separately.
[Preservation verification](../../Evidence/Candidate04/Verification/preservation.json)
confirms six recorded Candidate03 file identities, its local/public annotated
tag and all 15 inventoried performance payloads remain unchanged.

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

The recommended next milestone is focused weapon-handling, machine-contact and
HUD readability polish at the normal camera, with actual native held-input
playtesting and auditory review. It should add no new content or systems.
