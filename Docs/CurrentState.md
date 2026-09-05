# Current state — Candidate04

Candidate04 implements the M1911 starter, a 950-point Mystery Box, a
5,000-point Pack-a-Punch and independent Last Word, Overcurrent and Gravebreaker
variants within the existing arena. The catalog has six rows; inventory has
exactly two owned slots and supports a valid unarmed state. The complete rules,
balance values and controls are in the [pass report](Passes/Candidate04.md).

Gameplay source
[`8055041ebc98a4df7cd8923b05e7b89ad7372e38`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/8055041ebc98a4df7cd8923b05e7b89ad7372e38)
built the Editor, Win64 game and Development package from a clean public checkout.
It verified 875 tracked files and all 488 LFS payloads, totaling 233,654,563 bytes.
All nine native tests passed: six clean and three with five isolated test-world
teardown warnings. The four inventory tests produced no warnings. This verifies
a public checkout on the development host, not a clean operating-system install;
the final build used an incremental fast-forward from the initial C04 source.

The exact package passed all 15 modes with 984 assertions and zero failures;
every process exited with code 0. Its six required runtime hashes match the source build.
Progression passed 72 checks, Arsenal 148 and the legacy weapon suite 85.
Movement passed 112, including explicit slot/family checks for all 54 trials.
The initial legacy loadout-order failure and movement run with invalid family
coverage remain disclosed in the [pass report](Passes/Candidate04.md).
See the [evidence index](../Evidence/Candidate04/README.md) for per-mode counts,
fixture limits, source privacy coverage and build identity.

The final packaged walkthrough passed 58 checks and recorded 3,001 original
frames with 148.11733333 seconds of genuine engine WAV. Its last idle callback
arrived 9.795667 ms beyond the WAV end. The explicit
[assembly policy](../Evidence/Candidate04/Tools/README.md) preserves every raw
frame and its identity, presents 3,000 frames by excluding only `frame_03000.jpg`,
and retains 2.956479 seconds of final idle. Audio remains complete without
padding or stretching. The earlier 2,932-frame packaged attempt also passed
58 checks but failed strict assembly with a 34.454 ms endpoint overrun.

[Visual review](../Evidence/Candidate04/Verification/visual_review.json) inspected
75 original frames across both packaged captures: 68 dense and seven sparse.
Handoff, retrieval, machine forms and case flight read at the gameplay camera;
fine finger, shell-seat, slide and fore-end contact remain unapproved.
Intermittent missing HUD glyphs persist in JPEGs, and a capture-only cause is
unproven. The [23-phase audio measurement](../Evidence/Candidate04/Verification/audio_metrics.json)
found signal throughout the selected actions, a -14.632356 dBFS peak,
-41.983174 dBFS overall RMS and no full-scale samples or audit warnings.
No perceptual listening or user visual approval is claimed.

The [full movie](../Evidence/Candidate04/progression_loop.mp4) is 148.117333
seconds with 80 verified chapter titles and a successful complete decode.
Its [sidecar](../Evidence/Candidate04/progression_loop.json) binds the original
and presented captures. A stream-copy remux repaired chapter metadata while
preserving the encoded audio/video payloads. The
[67.9-second comparison](../Evidence/Candidate04/weapon_comparison.mp4) selects
eight base/upgraded and dim-machine excerpts, with eight verified chapter titles,
2,037 frames and a successful complete decode. Its
[sidecar](../Evidence/Candidate04/weapon_comparison.json) is the final source and
encoding record. These evidence-only assembly tools did not change or rebuild
the S2 runtime.

[Three completed profiles](../Evidence/Candidate04/Performance/README.md)
recorded 6/12/18 target-count combat at 1600x900, cap 120 and VSync off without
image/audio capture. Full mean frame times were 8.3350/8.3369/8.4123 ms;
p99 was 9.4036/9.3608/9.7091 ms. Exact target counts occurred in
75.01/71.42/78.40 percent of valid actor-counter samples, and both machines
were Active together for about 3.70 seconds per run. The workload fired actual
Last Word, replenished living infected and retained real corpses. It excludes
initial machine/weapon setup and does not profile carried Overcurrent or
Gravebreaker. Candidate03's stationary living-only reference and retained
screenshot spike differ; no causal improvement or GPU-cost claim follows.

[Ordinary native launch](../Evidence/Candidate04/native_controls.json) through
`Launch.ps1 -Candidate Candidate04 -Sandbox` exited the launcher successfully and
showed M1911 7/56, Empty slot 2 and zero points in the extracted package. A Windows
Security firewall prompt blocked gameplay input. It was left unchanged for the
user; no native gameplay events or video were produced. Only the two test game
processes were cleaned up. Sustained ordinary held-key playtesting remains
unverified, distinct from production-controller dispatch checks.

The [audited archive](../Evidence/Candidate04/Verification/archive.json) contains
1,217 entries and 407,635,789 bytes, with SHA-256
`1ad1a3258f16fccf12451fff829aa9124321491a457232b88efa871826ba783c`.
All 45 runtime files are identical to the audited fresh package; all six required
runtime identities match the source build. Candidate04 was extracted and verified
separately, preserving Candidate03.

The
[candidate04 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate04)
carries the archive and evidence. Its separate
[Candidate04-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate04/Candidate04-PublicationVerification.json)
attachment records checks performed after upload, including exact final commit,
downloaded hashes, full outgoing history audit and hydrated public checkout/LFS.
The built gameplay source remains distinct from the release documentation revision.

## Gameplay and ownership

Normal play starts with M1911 7/56 and slot 2 empty. Hold F for 0.4 seconds on an
in-range unobstructed machine. Release, target/range change, slot change, pause
or death cancels unfinished holds; another action requires release and a fresh
press. Visible prompts disclose price, action, reserved state and replacement.

The box chooses one eligible family at payment and shows actual assembled models
for five seconds. Weights begin equal and are editable. Reserved families are
excluded. Rewards wait indefinitely; collection revalidates the exact slot and
instance plan. Duplicate available families refill their effective variant
without creating a copy or downgrading it; full ammunition consumes the disclosed
result. Sandbox T grants 10,000 points; Z/X/C force the next family and V resets
random, while normal prices stay in force.

Pack-a-Punch reserves the same instance and slot and spends 5,000 atomically at
handoff acceptance. Preaccept cancellation is uncharged. Nine seconds include
intake, clamping, processing and output. The other available weapon remains
usable; depositing the only weapon warns of and enters an unarmed state. A box
reward can fill the other empty slot. Fresh collection restores that same slot
and instance once, upgraded and refilled, through normal equip. Upgrades have one
tier. Current-run technical failure before delivery restores the original weapon
snapshot and refunds once; death/reset invalidate old deliveries and refunds.

The original chest has a hinged lid, sliding locks, hydraulic struts and an
illuminated rotor. The processor moves the assembled weapon inward on its cradle,
clamps it between shields and rotating field rings, then brings the upgraded
assembly back to the output. Preview meshes use the actual weapon parts at
their authored scale. State lighting and bounded mechanical audio follow the
same machine cycle; collision and navigation use separate explicit shapes.

Pistol and rifle magazines drop from the evaluated seated prop at their removal
events, inherit movement, bounce and settle without blocking characters, shots
or navigation. Twelve actors maximum and eight-second lifetimes bound them.
Fresh hand props remain separate. Cancellation does not recreate an old magazine;
a loaded gun with its magazine removed requires R before firing. Tube shotguns
have no detachable magazine. Existing casings, pump obligations, sprint priority,
auto-reload and regional pellet resolution remain in use.

## Preserved foundation and limits

Candidate03's eight-direction 225/370 cm/s player movement, independent mouse aim,
stepping turns, 100/195 cm/s infected movement, regional damage, finite blood and
skeletal remains are retained. The same infected archetype, round/sandbox modes
and prior 96 map actors remain, with two powered machines and rebuilt navigation.
No map expansion, perks, extra enemy archetype or additional weapon family was added.

Prior character stride/pose limitations, fixed fingers, camera occlusion and
small-prop readability remain provisional. Runtime weapon and machine inspection
must be assessed at normal camera scale. Original synthesized sound banks and
recorded engine WAVs do not establish perceptual sound approval. The one-run
profiles are bounded measurements on the validation host; they do not establish
performance on other hardware or constant living-enemy counts.

The preserved [Candidate03 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
uses gameplay source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`, with final tag
commit `4aded688bbdb1a8bcfdc3ba1d6a6e3eb698c909a`. Its 398,043,452-byte Windows
archive SHA-256 remains
`ada689b2314d3047d581d0523a2cbcd91f57661d06554d6288a6b4fae79600d2`.
The [Candidate03 pass report](Passes/Candidate03.md) retains its distinct evidence.
[Preservation verification](../Evidence/Candidate04/Verification/preservation.json)
confirms its six recorded file identities and local/public tag remain unchanged.

The next focused milestone is weapon-handling, machine-contact and HUD readability
polish at the normal gameplay camera, including actual held-input playtesting
and auditory review. No new content or systems are proposed for that pass.
