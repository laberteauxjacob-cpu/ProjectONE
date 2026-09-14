# Candidate07 — infected physicality and recorded audio

Candidate07 develops one infected archetype into three recognizable appearances,
with hanging-arm locomotion, contextual attacks, physical crowd response and
recoverable living falls. Recorded physical shots, mechanisms, creature
performances and contact Foley replace the old physical-sound foundation.
Candidate06's player, six weapon variants, two-slot inventory, machines, health
regeneration, rewards and power-ups remain the gameplay baseline.

**Release status: Windows prerelease prepared; native control review is blocked and visual/audio approval remains provisional.** Technical verification and user
approval of appearance, sound and feel are separate.

## Release identity

| Item | Exact binding |
| --- | --- |
| Gameplay and asset source S | `3d75c2faa0cecef6075f00755d8e4f5dd2562817` |
| Publication revision D / review branch | [the candidate07 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate07) / `codex/candidate07` |
| Windows package | [ProjectONE-Candidate07-Windows.zip](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/ProjectONE-Candidate07-Windows.zip) |
| Archive size / SHA-256 | 414510213 bytes / `e3dae5b6ead801d03c6bc67362f8849ae465df88bd1e691ceed2681b18e61348` |
| S-to-D scope | Documentation, Candidate07 evidence and scoped `.gitattributes` transport rules only; gameplay, runtime assets and authoring tools remain byte-identical in Git to S. The later fresh-D verification independently checks this scope. |
| Fresh public checkout and LFS | Fresh S verified 1,816 tracked files and 1,132 LFS payloads / 449,311,411 bytes. Final D retrieval and its added evidence are recorded separately in the later publication-verification attachment. |
| Archive extraction and runtime identities | PASS: 1,230 ZIP entries, all 1,228 manifest payloads, CRCs, notice/prerequisite bytes and six extracted runtime identities verified; adopted to a separate Candidate07 folder. |
| Published refs, anonymous downloads and prior-candidate preservation | The [publication-verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/Candidate07-PublicationVerification.json) identifies actual D and records public refs, anonymous downloads, fresh D checkout/LFS and prior-candidate preservation when issued. These later public checks are not asserted by this committed text. |

The built runtime belongs to S. D may add reviewed documentation/evidence and
their attributes; it must not silently change gameplay or authoring tools after
the build. The later publication-verification attachment identifies actual D
after the public refs/downloads exist. Candidate06 source
`c2c422012f4f219fb5e63ac24294a6ba8428f999`, publication
`90ead259e10ef7ad7ed8803b0236aa02bb75eac6` and earlier candidates remain preserved.

## Appearance and connected combat

| Appearance | Source triangles | Main visual differences |
| --- | ---: | --- |
| Maintenance | 60,406 | Damaged blue jacket, gaunt face, receding swept hair |
| Laboratory | 64,798 | Lined split coat, lapels, alternate facial damage, grey hair |
| Facility Staff | 64,982 | Rolled cuffs, loose tie, brown trousers, broader lower face and broken dark crown hair |

Each has an editable Blender source and five modular exports, using the retained
21-bone infected bind. Each passed 140 evaluated source seam samples and weight
checks. Head, both arms and the existing supported left-leg sever remain tied
to the correct meshes and current pose. Source skin measurements fit the shared
head query; cosmetic hair does not enlarge it. The appearance data assets carry
no health, speed, damage or reward overrides. Player assets retain their own bind
and animation graph.

Fourteen infected-only clips provide locomotion, full-body turns, five sided
performances across swipe/rake/two-hand families, heavy hit, stumble and two
get-ups. Selection considers limbs, distance, bearing, entry speed and previous
performance. Bounded entry travel and recovery pursuit preserve approach
continuity without adding damage contacts. Effort and foot-contact events cross
explicit animation phases, matching the graph's explicit-time evaluation.

Upper-body physics yields to measured contact while the living capsule drives
locomotion. Full living falls retain health, registration and absent limbs and
cancel pending attacks. At most four living actors simulate full-body falls.
Floor support, upright clearance and the recovery path must remain valid; pickup
triggers cannot create support or obstruct a rise. Bounded physical effort can
help a fallen actor clear another actual fallen/dead body. Static blockers and
exhausted effort budgets can still prevent recovery.

Captured-pose rebasing precedes the complete prone/supine clip. Recovery lasts
2.90/3.35 seconds including its 0.35-second entry blend, keeps the same living
identity and cannot complete after death. The corpse/rest lifecycle and severed
parts remain bounded; a physical death uses its evaluated pelvis for drop origin.
The ordinary encounter recorded a sustained-contact fall, a 2.509 cm effort stopped for lost floor support, and a later prone recovery preserving 100.8 HP. A nearby corpse disappeared before clearance, so effort-caused escape is not established. The arranged physicality film completed two prone recoveries, retained four Fallen actors before phase cleanup and captured no supine recovery; its folded-to-prone entry remains conspicuous.

The regional ray wrapper fixes a reproduced Chaos cylinder-middle numerical miss
using the unchanged segment and exact evaluated capsule dimensions. It retains
world occlusion, ignore filters, removed-region exclusion and the existing
per-discharge victim transaction. This geometry correction does not redirect
mouse aim. [Runtime contract](../Candidate07Infected.md) and
[source/import notes](../../ArtSource/Characters/Candidate07/README.md) provide
the implementation detail.

## Recorded audio

The bank contains 134 processed cues from 76 retained originals. Shot reports,
reload/pump/extraction, performed creature voices, foot/body contacts and ambient
machinery use recordings. Weapon operation timestamps and actual contact events
remain authoritative. Priorities, cooldowns, attenuation and bounded shared pools
limit overlaps; event counts do not imply that every request was audible.

M4-family identity is not verified as an exact M4A1; the 870 uses recorded Nova
reports and SXP mechanisms. Object/wet-towel Foley supplies declared magazine,
impact and flesh analogues. Six Freesound originals are the official public MP3
representations. Some bank variants are alternate edits of the same performances.
Intentional upgrade energy is still synthesized, and machine/pickup accents
retain their earlier authored design.

[Credits, original hashes, licenses and recipes](../../ArtSource/Audio/Candidate07/README.md)
and the [runtime audio note](../Candidate07Audio.md) preserve those qualifications.
The grants cover the selected audio; no general project license is assigned.
Final old/new source-WAV comparison: [Full old-then-new PCM comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/reference_then_current.wav): 191.893333 seconds of verified C06 followed by 191.957333 seconds of verified C07, with no trimming, gain, added silence, resampling or crossfade. Both original packaged presentation fixtures passed 93 assertions. Actual mixer-stop clock is unavailable in that legacy format.
Full original PCM comparison and digital headroom measurements do not establish
realism, mix balance, localization or comfort. **No perceptual audition occurred.**

## Final verification

Every result below has its own source/runtime binding. Local editor
preflight passed 39 tests (36 clean, three with warnings), but that earlier result
does not substitute for the final fresh-S suite.

| Evidence | Final result and scope |
| --- | --- |
| Fresh neutral Editor/Game/package build | PASS: fresh public S checkout, UE 5.7.2 Win64 Development Editor/Game/package stages all exited 0. The original empty-stdout receipt correction and unchanged real engine log/report are retained in the build record. |
| All 39 engine tests at final S | PASS: 39 registered tests, 36 clean and three with warnings, zero failed or not run; the actual fresh-S report is retained. |
| Source/LFS/import/privacy audit and six runtime identities | PASS: 459 declared C07 payloads, all six runtime files and the recorded-source chain match. The audit retains all 25 classified binary-pattern hits across 19 files, the exact unchanged upstream-script example and untouched original notice bytes. |
| Packaged runtime matrix | PASS: all 32 runs completed. The 26 C07 runs comprise 13 regression/recording cases (6,867 driver assertions) and 13 recording-free profiles (502 driver plus 137 companion assertions): 7,506 assertions, zero failures. Six separate unchanged C06 profiles passed 228 assertions. These totals exclude the retained earlier low-plane failure and blocked native review. |
| Native input attempt | BLOCKED_NATIVE_CAPTURE: Windows returned cursor-access and monitor-capture errors after a fresh-window retry. Zero native actions or usable images; only the launched test process was terminated, and all six runtime files still match. Normal Quit and held-input review were not verified. |
| Original frames and twelve UI PNGs | 131 selected original chronological gameplay frames across four recordings, plus all 12 UI PNGs, were inspected with exact source/runtime/frame bindings. This is bounded still review, not continuous playback; the aim-height caption has a minor contrast issue over a pale floor stencil. |
| Four common recordings, full video/audio decode and original PCM measurements | PASS: four final engine recordings each passed all 32 inspection checks and full video/audio decoding. Combat, physicality, ordinary encounter and portability contain 2,380 / 2,413 / 3,292 / 867 encoded video frames with audio endpoints 79.317333 / 80.426667 / 109.717333 / 28.900000 seconds. Encoded counts include documented timing holds and differ from original capture-frame counts. Original PCM measurements, source-frame inventories, assembly timing and hashes are retained; this is not continuous viewing or audition. |
| Paired recording-free performance | All 19 recording-free workload analyses retain 66,511 original engine rows (44,389 C07; 22,122 C06) on the same RTX 3090/Ryzen 5950X host at 1600×900, cap 120, VSync off and D3D12. At 18 requested enemies, mean frame time rose from 8.4063 to 10.1898 ms for M4A1 and from 8.3484 to 10.4848 ms for 870. The C07 mixed 18-enemy run measured 10.6963 ms mean, 15.7497 ms p99 and 53.0143 ms maximum. All 19 maxima were first CSV rows; later spikes remain in the data. Actual live/state fractions, all distributions and single-sample/non-isolated host limits are retained in the performance evidence; no speedup or causal subsystem-cost claim. |
| Portable reports and original artifact inventories | [Candidate07 evidence](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate07/Evidence/Candidate07) |

The completed matrix has 32 serial runs: 13 C07 regression/recording runs, 13 C07
profiles and six unchanged packaged C06 baseline profiles. Actual counts, failures and assertion totals are retained separately for each candidate. The C07
regression set covers all six weapon bodies, 480 projected-aim discharges, 18
wall-grouping trials, survival/rewards, machine progression, 30/60/120 cadence,
two UI sizes, physicality, an ordinary encounter and three-appearance portability.
It does not imply every weapon × appearance × scenario combination was tested.

The combat, physicality and portability fixtures disclose direct setup/probes.
The ordinary encounter uses automated movement, logical cursor and fire/reload
input; it is not native human play. Common recordings retain actual full-camera
frames and original engine master audio. Their chronological inspection reports name actual frame identities and limitations. Decode success is not
continuous viewing, listening or visual approval.

Profiles pair packaged C06/C07 M4A1 and 870 at requested populations 6/12/18.
C07 additionally profiles explicit mixed falls at 1/2/6/12/18 with M4A1 and at
12 with Overcurrent and 870. The 1/2 workloads have no unchanged C06 equivalent.
Settings are 1600×900, cap 120, VSync off, screen percentage 100 and D3D12 with
recording disabled. The evidence retains all engine rows, duplicate non-identity column
positions, boundary rows, spikes and measured exact-live fractions. Disclosed
restoration/replenishment and direct fall probes limit the workload claim.
One sample per workload, changed behavior/assets/census overhead and uncontrolled
background/random variation cannot establish isolated subsystem cost, statistical
significance, universal FPS or spare capacity under a cap.

## Remaining limitations

The fixed low plane can miss floor-level bodies. A retained motion recording
processed ten real Left Ctrl + LMB shots at world Z 68.474 cm while the corpse's
body-query top was 61.743–62.698 cm; all ten missed. Its compatibility assertion
remains failed and is outside final passing-matrix totals; the
[retained record](../../Evidence/Candidate07/Verification/low_aim_compatibility.md)
preserves its WIP source limits and all ten shots. Direct posed-region
tests are not proof that ordinary mouse controls can target that height. No aim
snapping, larger hit regions or hidden target selection was added.

Dense bodies or solid obstructions may keep a living actor down; the final
encounter result above defines the observed scope. The initial get-up transition
is stylized, and occluded contacts and seams cannot all be judged from selected
frames. Native held-input coverage, continuous playback, perceptual audition and
clean-Windows prerequisite installation must not be inferred from other checks.

The final projected-aim log also reports a default-material fallback because
`M_EyeDark` lacks instanced-static-mesh usage. `ONE06ImpactSubsystem` uses that
material for finite world-impact cylinder marks; this warning does not identify
the Candidate07 infected-eye material. Its visual effect has not been assessed
by the behavioral log review. Startup/restart missing-mesh bone and CrowdManager
warnings remain in the raw logs; passing assertions do not make those logs
warning-free.

Recommend a focused, separately authorized floor-level targeting/usability pass.
Perk chambers, production hidden entrances and finished maps remain later work.
Project Zero is excluded, and Candidate07 does not add weapons or enemy classes.
