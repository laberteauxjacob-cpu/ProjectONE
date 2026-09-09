# Current state — Candidate05

**Local build, packaged checks, media and four profiles verified.** Packaged
source S is `6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Its clean public-source
Editor/Game/package build and all 17 engine automation tests pass. The default
suite passes 19 modes / 3,942 assertions; the supplemental suite passes 10 runs
/ 3,506 assertions, both with zero failures. Archive and local extraction match.

Publication is verified only by the [Candidate05-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/Candidate05-PublicationVerification.json) attachment, issued after actual public downloads and Git LFS checks. It identifies the reviewed [candidate05 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate05), documentation revision D, packaged source S and downloaded asset hashes. These pages report locally verified evidence until that attachment is issued.

Candidate05 continues the existing arena, enemies, regional combat, two-slot
inventory and Box/Pack-a-Punch economy. It replaces queued rejected shots with
eligible post-pose dispatch, commits magazine reloads through sprint/fire/switch
requests, and gives live hits, new kills and corpse hits distinct feedback.
Minor reactions preserve threat; heavy reactions and death can stop attacks.
Eighteen revised player clips, three stepping attack families, original rounded
HUD art, designed menus, spatial ambience/zombie voices and held upgrade effects
complete the requested presentation changes. See [controls](../README.md#controls)
and the [current combat rules](CombatRules.md) for the changed reload/input policy.

| Final-source evidence | Result |
| --- | --- |
| Fresh public build, LFS, engine automation tests, runtime hashes/privacy | PASS: [exact-S4 record](../Evidence/Candidate05/Verification/fresh_build.json); 1,129 tracked files, 648 LFS files / 297,457,501 bytes; 17 tests with five historical warnings; 45 runtime identities / 666,304,586 bytes |
| [Full 19-mode packaged suite](../Evidence/Candidate05/Verification/packaged_suite.json) | PASS: 19 modes, 3,942 assertions, zero failures; individual exit 0 and exact-source runtime binding |
| [Supplemental combat/UI checks](../Evidence/Candidate05/Verification/Supplemental/summary.json) | PASS: 10 runs / 3,506 assertions / zero failures; 30/60/120 cadence, 480 projected aim rays, four UI sizes and final motion/presentation captures |
| [Motion and presentation films](../Evidence/Candidate05/Media/README.md) | Encode and full decode pass: 1,389 / 5,257 source JPEGs; 51.179 / 196.950-second MP4 containers with actual engine audio. [Chronological image review](../Evidence/Candidate05/Visual/README.md) is bounded, with endpoint and perceptual limits |
| [Native desktop controls](../Evidence/Candidate05/Verification/native_input.json) | LIMITED PASS: 13 discrete actions / 16 trace events, six runtime hashes match; held movement and machine input remain untested |
| [Four recording-free rifle/crowd profiles](../Evidence/Candidate05/Performance/FinalS4/README.md) | All complete: M4A1/Overcurrent × requested 12/18, 14,937 numeric frames retained, mean 8.3500–8.5201 ms; every spike retained; non-isolated host |
| [Archive](../Evidence/Candidate05/Verification/archive.json) and local package | PASS: 1,217 entries, 409,708,689-byte ZIP, all runtime bytes match the fresh build; local extraction rehashed |
| [Earlier-candidate preservation](../Evidence/Candidate05/Preservation.json) | PASS before release for Candidate01–04; separate publication attachment will report final D, public downloads and LFS verification |

The verified local ZIP SHA256 is
`746cdc8d2299ba034b517e60bb519a599c3c6b84340d2700de6ccc4b39e8c8a4`.
The first native attempt stopped at the Windows Security prompt without sending
input or choosing network permissions. After the user handled it, the retry
verified one shot (7 to 6/56), reload (7/55), tools, pause/resume, restart
(0 points, 100 health, 7/56 and empty slot 2), and normal quit. A W tap did not
establish movement; it is included in the 13 actions, not a verified movement
result. This is not a continuous native playtest.

M4A1/Overcurrent are configured for 600/690 RPM. Their ideal burst DPS rises
60%, from 200/460 to 320/736, because of cadence alone. Final full-magazine
M4A1/Overcurrent rates at caps 30/60/120 are 599.997/684.780,
599.996/688.520 and 599.988/688.512 RPM. The
[timing record](../Evidence/Candidate05/Verification/weapon_timing.json) retains
frame quantization, a one-shot hitch recovery and finite-window limits.
The four profiles ran on the recorded Ryzen 9 5950X / GeForce RTX 3090 host,
Windows 11 25H2, actual 1600x900, cap 120, VSync off and screen percentage 100.
Their exact requested crowd count was present in 61.37–75.74% of counter frames;
replenished targets were not continuously held at 12/18. Maximum frame times
range from 18.8083 to 34.0449 ms. No locked-120-FPS, isolated GPU-cost or general
regression claim follows from one run per workload.

Player walk/sprint remain 225/370 cm/s, infected movement 100/195 cm/s,
strike damage 19 and protection 0.55 seconds. Two slots, M1911 7/56 start, prices
950/5000, cycles five/nine seconds and indefinite physical Ready rewards remain.

## Verification history and limits

Earlier source checkpoints are preserved in one sequence: S1's clean unity build
found a colliding lookup name; S2 fixed it but its legacy movement fixture tried
to switch during a committed reload, and its UI guard exposed actual 888x500
instead of requested 1600x900. S3 waited for reload completion and forced the
viewport; 15 modes passed, then three obsolete interruption/death expectations
failed in `ONEValidate`, leaving three benchmarks unrun. S4 changes those fixtures
to separately verify a minor hit preserving one contact, a meaningful heavy hit
cancelling contact and death preventing later contact from an independent
baseline. Production combat is unchanged by S4. Its completed `ONEValidate`
passes 44 assertions, and its full 19-mode suite passes independently of the
earlier failed attempts.

[Development evidence](../Evidence/Candidate05/README.md) keeps earlier cadence,
standing-height aim, UI and rendered rehearsals under their original source
scope. Elevated-target aim results are superseded for close standing-height
coverage. Rehearsal frames and audio are not final packaged media. Final
[four-size UI review](../Evidence/Candidate05/UI/four_sizes_review.md) viewed
all 24 original PNGs; the final UI/aura review inspected 32 consecutive windows,
303 distinct images and 15 full frames. No blocking settled UI/aura defect was
observed in that scope. Final chronological reviews retain mechanical joint
motion, similar swipe/rake silhouettes, a brief reel icon/name crossfade and
fine hand-contact/effect readability limits. Dim Last Word/Gravebreaker fire and
reload were not visually established; machine light obscures exact aura timing.

Perceptual audio listening is unverified: two attempts, including September 9,
confirmed that the tool cannot receive audio input.
[Numerical audio checks](../Evidence/Candidate05/Audio/README.md) cover the
51.178667-second motion and 196.949333-second presentation WAVs, both with zero
full-scale samples. The presentation generic phase-energy result remains FAIL:
pause phases 92/93 and death phase 96 contain only zero samples, consistent
with their recorded lifecycle context. No all-phase-audio PASS is claimed.
Waveform and cue checks cannot establish timbre, spatial balance or naturalism.
Offscreen scripted tests are not native input, measured foot lift is not movement
approval, and Candidate04's pistol profiles are not a matched rifle baseline.
Final recordings use actual engine audio; profiles retain spikes and disclose
their actual host conditions.

Candidate04's preserved public package uses reviewed D
[`2a75a4a5c09f52dacf289ced1b91d548ce3f2a3d`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/2a75a4a5c09f52dacf289ced1b91d548ce3f2a3d),
built S `8055041ebc98a4df7cd8923b05e7b89ad7372e38`.
Its [pass](Passes/Candidate04.md), [evidence](../Evidence/Candidate04/README.md),
tags, releases and package passed the S4 pre-release preservation checks.
That record explicitly does not verify Candidate05 publication.
The [Candidate05 pass](Passes/Candidate05.md) contains the issue/balance tables.
Recommend one next milestone: a focused human playtest and polish pass on
movement, attack feel and the actual audio mix.
