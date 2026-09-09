# Candidate06 — combat, survival, rewards and machine usability

**Publication status:** [Release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate06) / [final download-verification record](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json). Final verification is issued after publication; this committed text does not claim those later checks already ran.
Packaged gameplay source S3: `c2c422012f4f219fb5e63ac24294a6ba8428f999`.
Final publication/tooling revision D: [the `candidate06` tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate06).
Review branch: `codex/candidate06`; intended release tag: `candidate06`.
[Candidate05 comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate05...codex/candidate06).

This pass continues Project ONE's accepted arena, six weapon variants, two-slot
inventory, regional damage, movement and HUD. It addresses the Candidate05
playtest without starting the following infected/audio or perk-chamber milestones.

## Changes by requested issue

| Issue | Candidate06 behavior |
| --- | --- |
| Aim changes when an infected enters the cursor ray | One unshaken cursor-to-plane solution; enemy presence affects collision, not initial direction. Torso/head/low planes are 125/168/65 cm above the player's floor. |
| Grouping changes between walls and enemies | Private weapon random stream, editable movement/bloom/recovery, one sample per projectile and matched seeded occupancy fixtures. |
| Limited penetration and weak shotgun crowd response | All six profiles retain immutable rays, ordered victims, original range, damage loss and bounded body/contact counts. Eight independent shotgun pellets aggregate actual regional damage per victim; no area-damage substitute. |
| Head severing executes otherwise healthy targets | Head health damage uses 1.5×. Trauma alone cannot make a nonlethal hit lethal. A head kill requires more than half of the lethal packet's effective health damage to be head damage. |
| Inconsistent rewards | One 10-point impact per distinct living victim/discharge, plus 100 body or 120 qualified head kill points. Run/discharge/victim receipts reject duplicate and stale awards. |
| No player recovery or readable injury state | Regeneration begins after 15 seconds without accepted health loss at 10% of current maximum HP/s; red edges follow missing health. |
| Box wastes a purchase on an owned gun | Owned base/upgraded/reserved families are excluded. Empty/invalid forced pools do not charge; a paid ownership race refunds once without rerolling. |
| Upgrade deposit requires reload or precise alignment | Fresh F tap within clear front/side reach accepts full, partial, empty, reloading and pumping base weapons. Exact instance/slot reservation settles only earned events. |
| Ready collection disrupts the other gun | Automatic same-slot return supplies the established upgrade refill once without F or an exit/reentry maneuver; the other gun's reload and held-fire state are preserved. |
| Completed upgrades wait indefinitely | Ready starts a 15-second deadline with a final-five-second warning. Reachable collection wins exactly at the boundary; later arrival permanently loses that instance without refund. Technical rollback remains separate. |
| No dropped rewards | Distinct skull, ×2 and ammunition-crate pickups, real overlap collection, bounded reachable placement, timed HUD effects and three designed collection cues. |
| Weapon response needs restrained shake | Weapon-specific capped translation with 100%/50%/Off settings. Shake is applied after logical aim and cannot redirect shots. |
| Systems need reuse on future maps | Normal components and relocated machine actors work through the tiny Portability06 fixture; navigation and authored spawn registration remain explicit dependencies. |

Full input/lifecycle contracts are in [CombatRules](../CombatRules.md), with
[survival tuning](../Candidate06SurvivalRewards.md) and [controls](../../README.md#controls).
Only LMB fires. RMB selects head height, Left Ctrl selects low height and wins
if both are held; releasing them restores torso height. Wheel/Tab still cycle
weapons. Ordinary magazine reload commitment and non-buffered firing remain;
Pack-a-Punch transfer and Max Ammo are specific coherent transactions.

## Final tuning

Values are provisional gameplay choices, not claims about real ammunition.
Baseline infected health remains 112. Raw weapon damage is unchanged; stronger
crowd results come from real pellet paths, falloff and bounded penetration.

| Weapon | Bullet/pellet damage | Pellets | Range / falloff start | Damage at range | Total living bodies | Retained per prior body | Extra-body range |
| --- | ---: | ---: | --- | ---: | ---: | ---: | --- |
| M1911 | 28 | 1 | 2400 / 1000 cm | 55% | 2 | 60% | Full range |
| M4A1 | 32 | 1 | 2800 / 1400 cm | 65% | 3 | 65% | Full range |
| Remington 870 | 15 | 8 | 1400 / 500 cm | 20% | 2 per pellet | 60% | 650 cm |
| Last Word | 56 | 1 | 2400 / 1000 cm | 55% | 3 | 65% | Full range |
| Overcurrent | 64 | 1 | 2800 / 1400 cm | 65% | 4 | 70% | Full range |
| Gravebreaker | 30 | 8 | 1400 / 500 cm | 20% | 3 per pellet | 65% | 800 cm |

Falloff is linear. Extra-body range is measured from the original origin,
not the previous victim. Minimum accepted remaining damage is 2; configuration
is capped at five bodies and 16 contact queries per projectile. Solid world
geometry stops shots; corpse contacts are cosmetic and cannot award points.

| Weapon | Base spread | Movement contribution | Bloom per shot | Maximum | Recovery delay / degrees per second |
| --- | ---: | ---: | ---: | ---: | --- |
| M1911 / Last Word | 0.25° | 0.25° | 0.035° | 0.65° | 0.15 s / 2 |
| M4A1 | 0.35° | 0.30° | 0.055° | 1.15° | 0.16 s / 2.5 |
| Overcurrent | 0.25° | 0.30° | 0.055° | 1.15° | 0.16 s / 2.5 |
| Remington 870 / Gravebreaker | 4° | 0.40° | 0.10° | 4.7° | 0.25 s / 3 |

Movement contribution scales against horizontal walk speed, capped at 1.5×;
holstered bloom recovers too. M4A1/Overcurrent cadence stays 600/690 RPM.
The [final packaged timing report](../../Evidence/Candidate06/Verification/weapon_timing.md)
records the following full-magazine rates:

| Requested cap | M4A1 measured RPM | Overcurrent measured RPM |
| ---: | ---: | ---: |
| 30 | 599.997 | 684.779 |
| 60 | 599.996 | 688.419 |
| 120 | 599.987 | 688.511 |

These use the N−1 intervals between 24/36 actual discharges, excluding reload
time. Finite windows and available game frames explain deviations from the
configured 600/690 RPM. Maximum absolute Overcurrent phase error is
31.896/16.130/8.022 ms at the three caps. Each automatic weapon records one
recovery shot after the deliberate 320 ms stall and none in the short catch-up
guard. All six variants pass eligible same-frame and next-pose late taps.
This is component scheduling evidence, not native-input latency measurement.

| Rule | Final default |
| --- | --- |
| Player / infected speeds | Player 225/370 cm/s; infected 100/195 cm/s |
| Strike / player protection | 19 damage / 0.55 s |
| Start / inventory | M1911 7 loaded / 56 reserve; two slots, second empty |
| Head health / head kill classification | 1.5×; strictly more than 50% head contribution in the lethal effective-health packet |
| Combat points | Impact 10 per living victim/discharge; additional body kill 100 or qualified head kill 120; Double Points applies once to both |
| Regeneration | 15 accepted-damage-free gameplay seconds; 10% of current maximum HP/s |
| Box | 950 points; fresh 0.4 s holds to purchase/collect; five-second reveal; no owned families |
| Pack-a-Punch | 5,000 points; tap deposit; nine-second process; 200 cm clear front-anchor reach/front-and-side area; one tier |
| Ready deadline | 15 gameplay seconds; last-five-second warning; reachable exact-boundary collection wins, otherwise permanent loss/no refund |
| Natural drops | One 1% total roll per registered death; conditional weights 1/1/1 |
| Timed / world pickups | Insta-Kill and Double Points 30 s, refresh on recollection; uncollected lifetime 30 s; world expiry wins its exact boundary |
| Pickup limits | 80 cm collection sphere plus real capsule overlap/clear reach; cap 8 including collected-cue tails, editable 1–32 |
| Max Ammo | Effective loaded/reserve capacities for available owned weapons; no ownership creation or mutation of machine snapshots |
| Camera shake | Base pistol/rifle/shotgun impulses 0.55/0.32/1.15; upgraded ×1.15; amplitude cap 1.8; strength 100%/50%/Off |

Pause freezes gameplay clocks. Death/reset invalidates old effects, receipts
and ownership operations. Invalid PaP deposits neither cancel reloads nor charge;
valid deposits can leave a one-weapon player unarmed during processing.
The Box waits for ordinary magazine reload completion before collection.

## Final build and source-bound validation

The clean public-source S3 build completed Editor, Game and Windows package
with exit 0 using UE 5.7.2 (changelist 49658320), Win64 Development. All 28
source engine tests passed: 25 without warnings and three with five warning
events in total; zero failed or unrun tests.
Its 1,223 tracked files include 660 LFS payloads / 300,752,452 bytes; all source
payloads and six required runtime identities are bound by the fresh build record.
The eight Candidate06 source assets match their manifest, staged prerequisite
installer bytes match the engine, and tracked source remains unchanged afterward.
The separate runtime audit passed and binds this exact build record. It inventories
45 retained files / 666,721,590 bytes and marks four debug/manifest files for
omission. Raw retained bytes were scanned for ASCII/UTF16 user-directory roots,
with PE debug/signing metadata parsed; no actionable findings remain. One unchanged
engine diagnostic example is classified separately. Compressed cooked contents
are additionally covered by source-asset review; this is not a claim that raw
byte scanning exhaustively decodes them. Build and audit links:
[fresh build and its embedded runtime audit](../../Evidence/Candidate06/Verification/fresh_build.json). Archive/public-download verification is separate.

All 15 final S3 runs are complete: four capture fixtures, three weapon caps,
two UI sizes and six profiles. They total 4,084 assertions, zero failures and
exit 0 for every process, with source/runtime identity verified by each runner
before and after execution. The retained S1 results below are outside that
total. Performance and media results appear below; post-publication checks remain a separate record.

| Packaged fixture | Executed source, actual result and scope |
| --- | --- |
| Six-variant combat, actual standing head/leg contacts, misses, cover, rewards, crowd penetration and shake invariance | S3 PASS: exit 0, 2,110 assertions, zero failures; 2,095 captured frames. |
| Matched grouping with/without an infected at three distances | S3 PASS: exit 0, 922 assertions, zero failures; 18 trials and 1,135 captured frames. |
| Recovery, accepted/protected damage, drops, timed effects, Max Ammo and lifecycle | S3 PASS: exit 0, 100 assertions, zero failures; 1,735 captured frames. |
| Normal systems on relocated Portability06, scoring, overlap, healing, Box, upgrade return and deliberate expiry | S3 PASS: exit 0, 35 assertions, zero failures; 4,145 numerical timeline rows and 1,131 captured frames. Visual review separately inspects 40 unique frames. |
| Box eligibility/races and PaP acceptance, rollback, other-gun reload, return/deadline | Retained S1 PASS: exit 0, 95 assertions, zero failures; actual restart recorded. Not rerun at S3. |
| Ordinary projected aim across requested six variants/eight headings | Retained S1 PASS: exit 0, 2,708 assertions, zero failures; 480 actual shots cover six variants × eight headings × ten cases. Not rerun at S3. |
| Non-buffered firing, reload/mechanics and requested 30/60/120 caps | S3 PASS: exit 0 and 197 assertions per cap, 591 total, zero failures; 17,717 timeline rows read. |
| Rendered UI, actual button input, pause/death/restart/quit | S3 PASS: exit 0 and 49 assertions at each of 1600×900 and 1280×720, zero failures; all 12 original PNGs visually reviewed. |
| Recording-free profile executions: M4A1/870 × requested 6/12/18 | S3 PASS: six runs, exit 0 and 38 assertions each, zero failures. All measured frames and occupancy are reported below. |

The two retained runs executed at S1
`8b5f32640e1239f7035be4c7c1cc6713f0d8be8e`, with their original runtime
hashes and actual result/log/output bindings. The
[retained coverage record](../../Evidence/Candidate06/Verification/retained_coverage.md)
compares committed bytes: 119 of 121 source files are identical, including both
retained drivers and production aim, ballistics, input, GameMode, scoring,
health, power-up authority, machines and inventory. Config and Content are unchanged.
The four changed files are the pickup visual component, its design JSON/README,
and the separate weapon-timing fixture. This supports retention of the stated
S1 checks; it does not establish binary equivalence or an S3 runtime pass.

S2 multiplies the existing coloured pickup emission by 32 and moves the single
light nearer the floor, now explicitly set to 32 lumens with its 64 cm radius
unchanged. It also corrects the weapon-timing fixture's supposedly unowned
family selection to account for both inventory slots. The earlier S1 30-cap
attempt completed 197 assertions with two setup failures and remains failed
history; it is not retained as a pass. S3 then reflects pickup part positions
across Y and reverses their local yaw, preserving positive scales, after native
inspection exposed an upside-down skull and a numeral that read as ×5.
The [S3 source review](../../Evidence/Candidate06/Verification/source_review.md)
reports no actionable finding in the three-file, 12-insertion/one-deletion
change. It confirms unchanged controls and pickup authority, and explains the
geometry under the ordinary camera. It does not establish rendered acceptance
or native-input results. Actual S3 runtime and still-review results are listed
separately from the completed media and performance results below.

## Recordings, visual review, audio and native input

| Recording | Actual source frames | WAV duration | MP4 container / video / audio durations | Final artifact |
| --- | --- | --- | --- | --- |
| Combat | 2,095 | 79.125333 s | 79.133333 / 79.133333 / 79.125000 s | [candidate06_combat.mp4](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_combat.mp4) / [inspection](../../Evidence/Candidate06/Media/combat/inspection.json) |
| Grouping | 1,135 | 44.181333 s | 44.181000 / 44.166667 / 44.181000 s | [candidate06_grouping.mp4](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_grouping.mp4) / [inspection](../../Evidence/Candidate06/Media/grouping/inspection.json) |
| Survival/rewards | 1,735 | 66.688000 s | 66.700000 / 66.700000 / 66.688000 s | [candidate06_survival.mp4](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_survival.mp4) / [inspection](../../Evidence/Candidate06/Media/survival/inspection.json) |
| Portability | 1,131 | 44.842667 s | 44.842000 / 44.833333 / 44.842000 s | [candidate06_portability.mp4](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_portability.mp4) / [inspection](../../Evidence/Candidate06/Media/portability/inspection.json) |

All four original-source assemblies encode with exit 0 and pass 24 independent inspection checks each. Both video and audio streams are fully decoded to null with no seek/duration/frame limit and exit 0. Actual output is H.264, 1600×900, 30 FPS, full-range `yuvj420p` (JPEG range 2), with AAC 48 kHz stereo. Decoded video frame totals are 2,374 / 1,325 / 2,001 / 1,345 in table order; these are CFR output samples, not additional captured frames. The first captured image is held from audio time zero, later images follow actual callback timestamps, and the last image extends to the unchanged WAV endpoint within codec/container quantization. CFR sampling can duplicate or drop captured images. No crop, overlay, motion interpolation, replacement audio, synthetic silence or tempo change is used. Phase chapters come from the actual capture timeline. The screenshot callback clock starts after the recording call; unmeasured audio-render scheduling offset prevents a perceptual synchronization claim. [media, audio measurements and limits](../../Evidence/Candidate06/Media/README.md) binds the actual frame/WAV/assembly/movie identities.

Revision D adds documentation, evidence and their attributes, plus one movie-assembly command argument in `Scripts/assemble_candidate06_capture.py`: `-vf tpad=stop_mode=clone:stop_duration=1,fps=30`. The explicit last-image extension, trimmed at the unchanged WAV endpoint, corrects the first combat encode ending about 0.259 seconds before its audio. Original failed output and diagnostics remain preserved privately. This offline assembly correction changes no gameplay source or packaged runtime; the package remains the verified S3 build without a rebuild.

| Still-image review | Actually inspected scope and limits |
| --- | --- |
| [Combat](../../Evidence/Candidate06/Visual/Combat/combat_review.md) | 14 chronological sheets / 70 frame entries plus four full-original views, covering six-weapon head/low contacts and two shotgun crowd cases. No blocking issue in these short windows; full detached-part/corpse trajectories are not established. |
| [Grouping](../../Evidence/Candidate06/Visual/Grouping/review.md) | Three sheets / 18 post-discharge images, plus a nine-panel plot of all 234 contacts from 18 trials. Longer-distance walls are partly/off camera; measured grouping comes from recorded contacts, not the images. |
| [Survival](../../Evidence/Candidate06/Visual/Survival/review.md) | 32 original frames covering health, timed HUD motifs, actual collected-effect kill, pause, crate visibility and expiry/death cleanup. Early world crate is bright with a local glow; rapidly collected skull/×2 world geometry is covered by the separate native report. |
| [Portability](../../Evidence/Candidate06/Visual/Portability/review.md) | 40 unique frames: 36 on nine contact pages plus ten original views, six repeated. Box reward, tap prompt, return, Ready 15 s, final-five-second warning and permanent loss are readable. Early target and parts of the machine intake are obscured by framing/HUD. |
| Rendered UI | All 12 original PNGs, six at each tested size. Help/forced-pickup tray, compact HUD, pause/death and restart fit without a blocking layout finding. A small aim-height caption crosses floor lettering at the chosen cursor. [UI originals and review](../../Evidence/Candidate06/UI/review.md) |

These are source-bound still reviews with declared sampling and occlusion
limits, not continuous playback. Separate original views can repeat sheet
images; their counts must not be added into an invented unique-frame total.
The recorded checks supply exact health, scoring and transaction authority.

Original engine WAVs are 48 kHz, stereo, 16-bit PCM. Sample peak / RMS in dBFS for combat, grouping, survival and portability are respectively −12.405 / −38.294, −12.990 / −38.399, −16.500 / −45.344 and −17.747 / −40.915. All four have zero full-scale or near-full-scale samples and no numerical audit warnings. These source measurements do not establish AAC sample equality, timbre, localization or listening approval. No perceptual audio audition was performed. Full phase measurements and limits are linked in [media, audio measurements and limits](../../Evidence/Candidate06/Media/README.md).

The [S3 native report](../../Evidence/Candidate06/Verification/native_input.json)
records **LIMITED PASS: 14 discrete actions and 22 engine input edges**. LMB
changed M1911 ammunition from 7/56 to 6/56; RMB alone and Left Ctrl alone left
it unchanged. H and the three force-drop buttons worked without firing. Actual
native snapshots showed an upright orange-red skull, gold ×2, teal ammunition
crate and small local floor glows. Escape opened pause; Q exited normally, and
the six runtime identities remained unchanged afterward.

The firewall prompt was observed and testing resumed only after a fresh
snapshot showed it gone; no automated security change was made. No desktop
screenshot payload was saved or published. This check did not exercise held
modifier-plus-LMB combinations, native movement or machine approach, continuous
playback or audio audition. Engine-simulated input and the portability still
review remain separate evidence; no player approval of art or game feel is implied.

In-engine fixtures may declare teleports, frozen targets, sandbox grants and
forced drops. They use real engine frames and audio but are not uninterrupted
human survival play. Chronological still review must identify its actual scope;
do not describe it as continuous playback. Numerical audio measurements do not
establish timbre or audibility. Candidate05's limited native result and failed
audio-input attempts do not establish Candidate06 testing.

## Recording-free performance

Requested workloads are M4A1 and Remington 870 against 6/12/18 replenished
infected. All six profile executions completed successfully. Settings and actual
host condition: 1600×900, cap 120, VSync 0 and screen percentage 100, with matching available CSV/console readbacks; Windows 11 (25H2), AMD Ryzen 9 5950X. Ambient processes were recorded and the host was not isolated. Screenshot/audio recording was disabled.
Every measured frame and spike is retained. The table reports actual live counts,
machine overlap, world drops and timed-effect occupancy; requested counts
and startup modifiers did not remain present throughout every run.

| Weapon / requested enemies | Frames | Mean / P95 / P99 / maximum ms | Exact requested-live fraction | Actual machine/drop/effect occupancy |
| --- | --- | --- | --- | --- |
| M4A1 / 6 | 3,693 | 8.3399 / 8.8964 / 9.6056 / 21.2065 | 79.85% | 3.2168 s / 97.35% / 97.32% |
| M4A1 / 12 | 3,680 | 8.3709 / 9.0975 / 10.2173 / 24.9783 | 69.75% | 3.2112 s / 97.34% / 97.28% |
| M4A1 / 18 | 3,677 | 8.3750 / 9.0609 / 10.2795 / 31.4236 | 64.91% | 3.2172 s / 97.36% / 97.33% |
| Remington 870 / 6 | 3,695 | 8.3359 / 8.7096 / 8.9514 / 20.6116 | 93.26% | 3.2172 s / 97.35% / 97.29% |
| Remington 870 / 12 | 3,694 | 8.3398 / 8.7993 / 9.2964 / 25.5873 | 89.03% | 3.2085 s / 97.32% / 97.29% |
| Remington 870 / 18 | 3,690 | 8.3468 / 8.9135 / 9.4007 / 30.0013 | 86.28% | 3.2173 s / 97.34% / 97.32% |

The [full timelines, occupancy and host limits](../../Evidence/Candidate06/Performance/README.md) retain all 22,129 numeric frames with no spikes removed. An [independent headline check](../../Evidence/Candidate06/Verification/performance_headlines.json) confirms the six table summaries and payload identities. All six maxima occur at frame zero and remain included. Seven frames are strictly over 16.7 ms (two in M4A1/18 and one in each other run); none exceed 33.3, 50 or 100 ms. Exact-live and pickup fractions use valid counter rows, excluding one initial row without project counters per file. The requested crowd was therefore not continuously present. M4A1 commits 144 combat shots per run; the 870 commits 16. Both-machine Active time is separate from timed-pickup duration. The occupancy column lists both-machine Active seconds, then the measured fraction with at least three world drops, then the fraction with both timed effects. Effects and drops expire normally after setup. Inclusive machine/physics scopes overlap and are never summed or subtracted to claim a unique CPU cost; capped CPU frame times do not establish GPU attribution or improvement.

One host and one sample per workload cannot establish a universal frame-rate
guarantee or an isolated cost for one new feature. Recording overhead and
noncomparable historical workloads must remain separate from these samples.

## Distribution, editable sources and preservation

Archive: [ProjectONE-Candidate06-Windows.zip](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/ProjectONE-Candidate06-Windows.zip); 409,889,719 bytes; 1,217 entries; SHA256 `4dd954cdeb894d2be371fa78df0f640ef7a623685c40aa5cf0261c804f1c8b0a`. [local archive/extraction record](../../Evidence/Candidate06/Verification/archive.json) passes all entry CRCs, 1,215 manifest payloads and six runtime identities.
Actual public downloads: The release has eleven core asset roles: archive and two sidecars, plus four MP4/assembly-report pairs. The twelfth asset is [the final verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json), issued after the core downloads and fresh D checkout checks, then downloaded separately to confirm its bytes. This text does not claim those publication checks have already completed.
Final D public checkout/evidence/LFS: The [post-publication verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json) records the final tag/branch/main identity, fresh D checkout and source/evidence LFS verification when issued; no final D result is asserted here.
Candidate01–05 tag and Candidate05 release preservation: The local archive verification confirms the Candidate05 archive remains unchanged. The [post-publication record](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json) separately reports Candidate01–05 tag identities and Candidate05 release/body/asset preservation after publication.

The [source asset inventory](../../Evidence/Candidate06/SourceAssets.json)
records three original PCM cues, their three SoundWave imports, the pickup
material and Portability06 map. [Provenance](../Provenance.md),
[pickup design](../../ArtSource/Pickups/Candidate06/README.md) and
[audio sources](../../ArtSource/Audio/Candidate06/README.md) retain editable
scripts and definitions. Pickup geometry uses engine primitives. The cues are
intentional synthesis; they are not recorded physical sounds. No Project Zero,
external packs, purchases, paid services or broad licensing change are introduced.

This remains a development sandbox with provisional tuning. Finite still-image samples, numerical fixtures and one non-isolated profile per workload do not establish uninterrupted human play, a universal frame-rate guarantee or player approval. Native held firing combinations, movement feel and perceptual audio audition were not tested. The new pickup cues are synthetic; the accepted infected visual/physical and recorded-audio milestone remains future work.

## Next milestone

Recommend the accepted infected visual/physical redesign and recorded-audio
replacement: decayed human archetype/compatible variants, relaxed arm poses,
flowing pursuit/attacks, expandable distinct attacks, controlled crowd contact,
stumbles/toppling/recovery, navigation and limb-loss preservation, and suitable
recorded gunfire/mechanisms/footsteps/clothing/casings/creature voices with
redistributable source terms. Upgraded electronic layers remain appropriate.

The following six perk chambers remain in [Roadmap](../Roadmap.md): Juggernog
(maximum health), Speed Cola (coherent faster reloads), Double Tap (synchronized
fire rate), Quick Revive (solo self-revive), Stamin-Up (sprint) and Mule Kick
(third slot). Physical entry grants protection only during a legitimate cycle;
a bounded exit blast/knockback creates room. Invalid/repeated purchases cannot
grant free invulnerability. Prices, strengths, duration, revive consumption
and loss rules remain undecided. Future maps use hidden spawn → occluded
approach → visible entrance → pursuit; visible sandbox spawning is a testing
exception. Neither future milestone begins automatically.
