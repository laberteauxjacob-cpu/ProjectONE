# Candidate05 evidence

**Local source, package and final evidence verified.** Source S is
`6b8621cef9d2a87de6e6eabc1743359c6274da5a`. The fresh build, 17 engine
automation tests, default 19-mode / 3,942-assertion suite and supplemental
10-run / 3,506-assertion suite pass. Both films pass full decode; four
recording-free profiles complete. Native input and image/audio reviews have
the specific limits below.

See [play and controls](../../README.md), [current state](../../Docs/CurrentState.md)
and the [pass report](../../Docs/Passes/Candidate05.md).

Publication is verified only by the [Candidate05-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/Candidate05-PublicationVerification.json) attachment, issued after actual public downloads and Git LFS checks. It identifies the reviewed [candidate05 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate05), documentation revision D, packaged source S and downloaded asset hashes. These pages report locally verified evidence until that attachment is issued.

## Final-source records

| Record | Status and scope |
| --- | --- |
| [Fresh build / engine automation / runtime](Verification/fresh_build.json) | PASS: UE 5.7.2; 648 LFS files / 297,457,501 bytes; 17 tests with five historical warnings; 45 runtime identities / 666,304,586 bytes |
| [Default packaged suite](Verification/packaged_suite.json) | PASS: 19 modes / 3,942 assertions / zero failures; individual exits and exact-source runtime binding |
| [Supplemental suite](Verification/Supplemental/summary.json) / [raw inventory](Verification/Supplemental/raw_file_inventory.json) | PASS: 10 runs / 3,506 assertions; cadence, 480 projected aim rays, four UI sizes and both final captures |
| [Weapon timing](Verification/weapon_timing.json) | PASS: 191 checks per cap; M4A1 599.99–600.00 RPM and Overcurrent 684.78/688.52/688.51 RPM full-magazine rates at 30/60/120 caps |
| [Movie verification](Media/README.md) / [identity and timing](Media/verification.json) | Encode/full decode PASS; 1,389 / 5,257 original callbacks, actual engine audio, 51.179 / 196.950-second MP4 containers; endpoint differences disclosed |
| [Chronological image reviews](Visual/README.md) | Bounded motion/stride, six-weapon and UI/aura inspections of original final packaged frames; no continuous-playback or user-approval claim |
| [Four-size UI review](UI/four_sizes_review.md) / [image identities](UI/four_sizes_review.json) | All 24 actual PNGs viewed at 1280x720, 1600x900, 1920x1080 and 2560x1080 |
| [Native desktop controls](Verification/native_input.json) | LIMITED: 13 actions / 16 trace events and six matching runtime hashes; W tap did not establish movement, held machine input untested |
| [Recording-free profiles](Performance/FinalS4/README.md) / [summary](Performance/FinalS4/summary.json) / [inventory](Performance/FinalS4/inventory.json) | Four complete M4A1/Overcurrent × requested 12/18 runs; 14,937 numeric rows and all spikes retained, non-isolated host |
| [Audio measurements](Audio/README.md) | Numerical master-WAV review; presentation generic phase energy FAIL retained for silent pause/death phases; no perceptual audition |
| [Archive verification](Verification/archive.json) | PASS: 1,217 entries, 409,708,689 bytes, CRC/manifest and runtime match; local extraction rehashed |
| [Prior-candidate preservation](Preservation.json) | PASS before release for Candidate01–04; not Candidate05 publication verification |

[Windows ZIP](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/ProjectONE-Candidate05-Windows.zip) · [Motion film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/motion_review.mp4) · [Presentation film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/presentation_review.mp4)

Local ZIP SHA256:
`746cdc8d2299ba034b517e60bb519a599c3c6b84340d2700de6ccc4b39e8c8a4`.

The default suite's six original 1600x900 stills have a [separate review](UI/portable_review.json).
Four original PNG copies are included: [starter](UI/starter.png),
[tools](UI/sandbox_tools.png), [pause](UI/pause.png), [game over](UI/game_over.png).
The later four-size review independently inspected all 24 images; default
copies are not relabeled as supplemental images. No blocking glyph/value
clipping or unintended overlap was observed within the six states per size.

The [UI/aura review](Visual/ui_aura_review.md) covers 32 consecutive windows,
303 distinct images and 15 full frames. Settled prompts, Hold F, menu transitions
and held effects show no blocking defect within scope. Brief reel icon/name
crossfade, restrained effects, machine-light occlusion and unreviewed dim
Last Word/Gravebreaker fire/reload remain explicit. The
[motion/stride review](Visual/Motion/review.md) and [weapon review](Visual/weapons_review.md)
retain mechanical motion, similar attack silhouettes and small or occluded hand
contacts. Overlapping review counts are not added into a unique-frame claim.

Configured rifle cadence is 600/690 RPM. The 60% ideal burst-DPS increase is
a disclosed tuning change, not a measured performance claim. Final profile
mean frame times are 8.3500–8.5201 ms on the recorded Ryzen 9 5950X / RTX 3090
host at actual 1600x900 and cap 120. The requested crowd count was held in only
61.37–75.74% of counter frames. Every spike remains; no locked-120-FPS, matched
Candidate04 rifle baseline or isolated GPU-cost claim follows.

## Provenance and earlier development records

- [Art/audio provenance](ArtAndAudioProvenance.json): authored inputs and source identity.
- [UI import](UIImport.json): nine original HUD textures, including actual weapon-assembly renders.
- [Audio import](AudioImport.json): 48 original sounds and import results.
- [Weapon timing](WeaponTiming.md): development-only six-variant runs at three requested caps.
- [Functional integration](FunctionalIntegration.json): development results, failed iterations and raw report hashes; elevated-target aim coverage is explicitly superseded.
- [Visual review](VisualReview.md): bounded development-frame inspection with source scope and concrete limitations.
- [Preservation](Preservation.json): S4 pre-release checks for Candidate01–04; Candidate05 publication verification is explicitly separate.

The [pass report](../../Docs/Passes/Candidate05.md) consolidates the failed S1
unity build, S2 movement/viewport assumptions and S3 attack-fixture expectations.
Their passing subresults remain historical; S4 introduces separate minor-contact,
heavy-interruption and death checks. S4 `ONEValidate` passes 44 assertions and
the full default suite passes independently of those historical failures.

Private logs and raw regenerable captures stay outside source history. Genuine
engine media, numerical audio checks, perceptual listening and native input are
distinct evidence. [Audio measurements](Audio/README.md) bind the final motion
and presentation WAVs, with peaks -15.052/-14.795 dBFS and zero full-scale samples.
Motion signal checks pass; presentation generic phase energy remains FAIL for
all-zero pause phases 92/93 and death phase 96, with lifecycle context preserved.
[Verification methods](Verification/README.md) preserve the private report-parser
and chapter-CSV failures without modifying source gameplay or raw captures.
Audio listening is unverified: two capability attempts,
including September 9, confirmed that the tool cannot receive audio input.
A nonclipping waveform does not establish heard timbre, localization or balance.
Recording-free profiles retain spikes and disclose fixtures/host conditions.
Technical verification does not imply user approval of art or game feel.
