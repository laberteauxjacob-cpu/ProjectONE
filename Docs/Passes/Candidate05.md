# Candidate05 — responsive combat and presentation

**Local package and final evidence verified.** Packaged source S is
`6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Its fresh public-source build,
17 engine automation tests, runtime audit, default 19-mode / 3,942-assertion suite
and supplemental 10-run / 3,506-assertion suite pass. Both films pass encode and
full decode; all four recording-free rifle/crowd profiles complete. Limited
desktop input and bounded chronological image reviews are reported separately.

[Windows package](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/ProjectONE-Candidate05-Windows.zip) · [Motion film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/motion_review.mp4) · [Presentation film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/presentation_review.mp4) · [Prerelease page](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate05)

See [controls and launch](../../README.md), [current state](../CurrentState.md)
and [evidence index](../../Evidence/Candidate05/README.md).

Publication is verified only by the [Candidate05-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/Candidate05-PublicationVerification.json) attachment, issued after actual public downloads and Git LFS checks. It identifies the reviewed [candidate05 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate05), documentation revision D, packaged source S and downloaded asset hashes. These pages report locally verified evidence until that attachment is issued.

## Final verification

| Evidence | Final-source result |
| --- | --- |
| [Fresh build and engine automation tests](../../Evidence/Candidate05/Verification/fresh_build.json) | PASS: UE 5.7.2 / Win64 Development; 1,129 tracked files; 648 hydrated LFS files / 297,457,501 bytes; 17 tests with five warnings in three historical fixtures |
| Runtime privacy/hash audit and prerequisites | PASS: 45 runtime identities / 666,304,586 bytes, no unclassified findings, both prerequisites and six-file exact-build binding |
| [Default packaged suite](../../Evidence/Candidate05/Verification/packaged_suite.json) | PASS: 19 modes / 3,942 assertions / zero failures; every exit 0 and actual 1600x900 |
| [Supplemental checks](../../Evidence/Candidate05/Verification/Supplemental/summary.json) | PASS: 10 runs / 3,506 assertions / zero failures; cadence, four UI sizes, projected aim and both final captures. [Raw inventory](../../Evidence/Candidate05/Verification/Supplemental/raw_file_inventory.json) binds identities |
| [Movie verification](../../Evidence/Candidate05/Media/README.md) | Encode/full decode PASS for both actual S4 films, 1600x900 H.264 / 30fps, AAC 48kHz stereo. Image review and audio limits remain separate |
| [Native desktop input](../../Evidence/Candidate05/Verification/native_input.json) | LIMITED: 13 actions / 16 trace events, six runtime hashes match; the W tap did not establish movement |
| [Recording-free profiles](../../Evidence/Candidate05/Performance/FinalS4/README.md) | Four complete runs, all 136 fixture assertions pass; all 14,937 numeric frame rows and spikes retained |
| [Archive](../../Evidence/Candidate05/Verification/archive.json) and local adoption | PASS: 1,217 entries comprising 45 runtime files, 1,169 notices and three distribution documents; CRC/hash verification and extracted runtime match |
| [Earlier-candidate preservation](../../Evidence/Candidate05/Preservation.json) | PASS before release for Candidate01–04; explicitly separate from Candidate05 publication verification |

The verified ZIP is **409,708,689 bytes**, SHA256
`746cdc8d2299ba034b517e60bb519a599c3c6b84340d2700de6ccc4b39e8c8a4`.
Local extraction verifies 1,217 files without replacing an older package.
Editor/Game/package builds exit 0 in 21.828/22.969/21.750 seconds; these are
build times, not gameplay performance. All eight Candidate05 engine tests are
warning-free. Four private/debug runtime files were omitted; one unchanged
upstream diagnostic example is explicitly classified.

The first native attempt stopped at a Windows Security prompt without input or
a permission action. After the user handled it, the retry verified Round 1,
F1 sandbox, one shot (7 to 6/56), R reload (7/55), H tools, Add 10000/Back
without ammo change, pause/resume, restart (0 points, 100 health, 7/56 and empty
slot 2), and normal quit. The 13 actions include the inconclusive W tap.
Continuous held movement, held machine input and native recording remain
untested; no desktop/security image is included in public evidence.

Final [weapon timing](../../Evidence/Candidate05/Verification/weapon_timing.json)
passes 191 checks at each cap. Full-magazine M4A1/Overcurrent rates are
599.997/684.780 RPM at 30, 599.996/688.520 at 60 and 599.988/688.512 at 120.
Overcurrent alternates frame gaps 2/3, 5/6 and 10/11; finite-window rates differ
from configured 690 RPM. All six variants pass eligible pre-actor same-frame
and post-actor next-pose dispatch checks. A 320 ms injected hitch produced one
recovery shot and zero immediate catch-up shots before deliberate input flush;
this does not establish sustained post-hitch behavior or native-input latency.

## Recording-free performance

Actual runs used a Ryzen 9 5950X / NVIDIA GeForce RTX 3090 host, Windows 11 25H2,
UE 5.7.2 Development, actual 1600x900, cap 120, VSync 0 and screen percentage 100.
Ambient processes were recorded; no isolated-host or matched-baseline claim is
made. Acquisition and upgrade setup finish before measurement. Captures include
physical pistol deposit, both machines active for about 3.70 seconds, carried
rifle bursts/reloads, replenished enemies, physics/debris and explicit
profile-only health protection. Screenshot/audio recording and encoding were
separate from these measurements.

| Rifle / requested live | Frames | Mean ms | p95 ms | p99 ms | Max ms | Exact-live fraction | Combat shots |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M4A1 / 12 | 3,760 | 8.3500 | 8.9213 | 9.6114 | 25.3392 | 75.74% | 144 |
| M4A1 / 18 | 3,684 | 8.5201 | 9.5430 | 10.3240 | 34.0449 | 71.63% | 144 |
| Overcurrent / 12 | 3,759 | 8.3533 | 9.1223 | 9.8786 | 18.8083 | 65.14% | 180 |
| Overcurrent / 18 | 3,734 | 8.4065 | 9.2134 | 10.1402 | 22.7024 | 61.37% | 180 |

Each workload retains one frame over 16.7 ms; M4A1/18 also has one over 33.3 ms.
None exceeds 50 or 100 ms in these runs. Each maximum occurs in frame 0, the
retained capture transition; its cause is not assigned from these measurements.
The exact requested live count was
present in only 61.37–75.74% of counter frames, so the crowd was not continuously
held at 12/18. One row per run has no project counters but remains in frame-time
statistics. The [complete analyses and timelines](../../Evidence/Candidate05/Performance/FinalS4/README.md)
retain all spikes and full/10–20-second/machine-overlap/combat selections. A
120 FPS cap is not locked 120 FPS. One run per workload establishes neither
general regression freedom, isolated GPU cost nor a causal improvement.

## Final media

The [motion film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/motion_review.mp4) contains 1,389 source callbacks
and 1,523 encoded/decoded frames; the [presentation film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/presentation_review.mp4)
contains 5,257 callbacks and 5,897 encoded/decoded frames. MP4 container lengths
are 51.179 and 196.950 seconds; original engine WAVs are 51.178667 and
196.949333 seconds. Held original images are resampled to 30fps without motion
interpolation or replacement audio. Source callback rates are about 27.35/26.74
per second and are not a game-performance measurement.

[Media verification](../../Evidence/Candidate05/Media/verification.json) binds
movie/sidecar/WAV/ledger hashes and the completed full decodes. Video tracks end
about 0.412/0.383 seconds before their containers; implicit last-image display
during that audio remainder was not checked through continuous playback.
The films contain no embedded chapters. Codec readability is separate from
the bounded visual and numerical audio evidence below.

## Changes driven by playtest

| Reported issue | Candidate05 behavior |
| --- | --- |
| Near cursor reverses shots/traces | Shared character-centered intent, four-centimeter fallback, exact valid forward convergence and bounded contact traces; real cover remains authoritative |
| Clicks execute after the gun becomes available | Only an eligible press gets one post-pose dispatch; rejected input is discarded |
| Rifle feels slow and separated | M4A1 0.100-second interval and matching fire operation; Overcurrent retains its 15% rate advantage |
| Reload interruptions leave confusing states | Magazine reloads commit through sprint/fire/switch requests; inventory and machine backdoors reject until a fresh valid request |
| Shotgun loading/input ambiguity | Earned shells remain; closing the load requires safe hand return and a fresh eligible shot, with no hidden queue |
| Weak first-hit response | Animated live-hit marker, distinct kill shape, directional blood/audio and additive localized reactions; corpse hits cannot award another kill |
| Deep crouching and mechanical strides | Eighteen revised player walk/run/turn clips, less knee compression and restrained torso weight shift on the existing rig |
| Zombie stops and waves an arm | Stepping swipe, cross-body rake and two-hand families with anticipation, fixed heading, bounded step, contact and recovery |
| Small glyphs and permanent text panels | Original rounded artwork, prominent ammo/yellow points, health bar, actual weapon images, compact slots and contextual labels |
| Permanent sandbox paragraph | Compact badge and grouped H tray; existing F1 meaning remains |
| Box reward unclear from camera angle | Reel uses actual assembled weapon images and the existing paid preview/result; ready state shrinks and directs return to the box |
| Plain pause/death rectangles | Shared style, actual scene dimming, large results and mouse actions; death beat is brief and immediately skippable |
| Silent environment/enemies | Original spatial mechanical room beds, sparse cues and varied breathing/pursuit/attack/hit/death sounds with bounded groups |
| Upgrades lose identity while carried | Close violet/cyan/ember rails and light follow the held gun; stronger flash/tracer colors and layered energy shot variations |

The [combat rules](../CombatRules.md) make eligible/rejected input, committed
magazine reloads, earned shotgun shells, collision priority and feedback outcomes
explicit. Passing checks do not imply visual or auditory acceptance.

## Retained balance and tuning

| Weapon | Loaded / initial reserve | Damage before regional modifiers | Configured interval | Nominal burst DPS before falloff/reload |
| --- | ---: | ---: | ---: | ---: |
| M1911 | 7 / 56 | 28 | 0.240 s | 116.67 |
| Last Word | 14 / 112 | 56 | 0.208696 s | 268.33 |
| M4A1 | 24 / 192 | 32 | 0.100 s | 320 |
| Overcurrent | 36 / 384 | 64 | 0.086957 s | 736 |
| Remington 870 | 6 / 36 | 8 × 15 pellets | 0.780 s including fire/pump | 153.85 if all pellets hit |
| Gravebreaker | 8 / 72 | 8 × 30 pellets | 0.678261 s including fire/pump | 353.85 if all pellets hit |

M4A1/Overcurrent change from 0.160/0.139130 seconds to configured 0.100/0.0869565
seconds, or 600/690 RPM. Their ideal burst DPS increases **60%**, from 200/460
to 320/736. Damage, capacities, reserve limits, spread/recoil, upgrade multipliers
and special behavior retain Candidate04 tuning. These are ideal calculations,
excluding falloff and reload; Last Word retains one extra victim at 60% damage,
and shotgun regional pellets aggregate once per victim/discharge.

Player walk/sprint remain 225/370 cm/s, infected movement 100/195 cm/s, strike
damage 19, initial range 88 cm and protection 0.55 seconds. The game starts with
M1911 7/56 and an empty second slot. Two slots, one upgrade tier, Box/PAP prices
950/5000, five/nine-second cycles and indefinite physical Ready rewards remain.
Swipe/rake/two-hand contact times are 0.45/0.48/0.54 seconds, clip durations
0.96/1.08/1.12 seconds and maximum steps 18/12/14 cm. Fixed commitment heading,
geometry sweeps, required limbs and one-contact state govern attack fairness.

## Failed checkpoints and development evidence

S1 `183a86ca3e7e` exposed a `Directions` unity-build name collision. S2
`e0e203a41e16` fixed it and passed build/engine-test/runtime checks, then failed an old
movement fixture's reload-time switch and a UI viewport guard at actual 888x500
instead of requested 1600x900. S3 `3c31d09a387f` corrected fixture timing and
forced resolution; its first 15 modes passed at 1600x900, but `ONEValidate`
reported three interruption/death failures and benchmarks 6/12/18 were not run.
The old fixture assumed any hit interrupted an attack and reused health from
before a legitimate contact. S4 changes only validation fixtures: a minor hit
must preserve its attack clock and exactly one damaging contact, a real
catalog-threshold nonlethal heavy hit must cancel contact, and death must prevent
later contact against fresh baselines. S4 `ONEValidate` passes 44 assertions,
including the new checks; its entire 19-mode default suite passes. Failed attempts
remain preserved; successful rows from different sources are never merged into
a final-source suite pass.

Earlier [functional integration](../../Evidence/Candidate05/FunctionalIntegration.json),
[weapon timing](../../Evidence/Candidate05/WeaponTiming.md) and
[moving review](../../Evidence/Candidate05/VisualReview.md) retain their original
development scope. They include corrected standing-height aim, four actual UI
sizes and motion/presentation rehearsals. Elevated-target aim passes are
superseded for standing-height coverage; rehearsals are not final packaged films.
Original sources comprise 25 animations, nine UI textures and 48 sounds, with
their [provenance and imports](../../Evidence/Candidate05/README.md) recorded.

## Methods and remaining limits

Scripted Controller dispatch, offscreen rendering and native desktop input are
separate methods. Fixed-step aiming checks establish collision behavior, not
real-time firing latency. Explicit loadout/health/target fixtures are disclosed
per run. Final projected aim covers 480 rays; low-cover shoulder/muzzle blocks
produce no victim or tracer behind the obstruction. Measured foot clearance
alone does not prove natural locomotion. Final chronological reviews retain
mechanical knee/foot transitions, rigid upper aim, abrupt pivots, similar
swipe/rake silhouettes and small or occluded mechanism/hand contacts.

The [four-size UI review](../../Evidence/Candidate05/UI/four_sizes_review.md)
viewed all 24 actual PNGs at 1280x720, 1600x900, 1920x1080 and 2560x1080, covering
starter 0/7/56, 1500 points, slots/H tray, pause, death and restart. No blocking
glyph omission, clipped value or unintended overlap was observed. The separate
[UI/aura review](../../Evidence/Candidate05/Visual/ui_aura_review.md) inspected
32 consecutive windows / 303 distinct images / 15 original full frames, including
paid reels, settled Ready, Hold F, handoff, bright upgraded shots/reloads, dim
Overcurrent, tools, pause/resume and game over. It found no blocking settled
UI/aura defect within that scope. The reel briefly crossfades an incoming name
over an outgoing icon; effects are restrained, machine light obscures exact
aura/swap timing, and dim Last Word/Gravebreaker fire/reload were not visually
established. Static chronological inspection is not complete film playback.

Perceptual audio review is unverified: both capability attempts, including the
September 9 retry, reported that the tool cannot receive audio input. The retry
used an existing development mix, not final-source audio. The final engine WAVs
are 51.178667 seconds (motion) and 196.949333 seconds (presentation), 48 kHz stereo
16-bit PCM. [Numerical audio checks](../../Evidence/Candidate05/Audio/README.md)
measure peaks/RMS of -15.052/-46.498 dBFS and -14.795/-41.079 dBFS respectively,
with zero full-scale samples. Motion signal checks pass. Presentation's generic
phase-energy result remains FAIL because pause phases 92/93 and death phase 96
contain only zero samples; their captured lifecycle context is documented rather
than relabeled PASS. The films retain the original game mix. Numerical events
and waveforms do not establish heard timbre, localization, naturalism, true-peak
headroom or perceptual sync.

The [verification methods](../../Evidence/Candidate05/Verification/README.md)
preserve the initial projected-aim report parser failure and strict chapter-CSV
attempt. Corrected evidence parsing leaves game/raw data unchanged; the movies
use no embedded chapters, with separate phase/time labels and original hashes.

Profiles retain every frame and spike, disclose replenished targets/health
protection and actual host conditions, and run without recording. Overlapping
CPU scopes are not added. Candidate04's pistol workloads are not a matched
baseline for these rifle workloads; no causal speedup or general absence of
performance regression is inferred. Fresh builds verify the public source on
the validation host, not a clean Windows installation.

Candidate04 and earlier tags, releases and packages passed the S4 pre-release
preservation checks. The separate release-verification attachment will report
Candidate05 publication and actual downloaded asset identities. One proposed next milestone is a focused human
playtest and polish pass on movement, attack feel and the actual audio mix.
