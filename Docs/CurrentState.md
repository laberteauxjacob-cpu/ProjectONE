# Current state — Candidate06

Candidate06 implements the requested combat, survival, rewards and machine
changes on the accepted Project ONE foundation. Packaged gameplay source S3 is
`c2c422012f4f219fb5e63ac24294a6ba8428f999`; final publication revision D is
[the `candidate06` tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate06). Review branch: `codex/candidate06`.
**Release and actual-public-download status:** [Release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate06) / [final download-verification record](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json). Final verification is issued after publication; this committed text does not claim those later checks already ran.

The [Candidate06 pass](Passes/Candidate06.md) records issue coverage, final
tuning, source-bound evidence and limitations. [Controls](../README.md#controls),
[combat rules](CombatRules.md) and [survival/rewards tuning](Candidate06SurvivalRewards.md)
describe current behavior. Historical passes retain their original rules.

Revision D adds documentation, evidence and their attributes, plus one movie-assembly command argument in `Scripts/assemble_candidate06_capture.py`: `-vf tpad=stop_mode=clone:stop_duration=1,fps=30`. The explicit last-image extension, trimmed at the unchanged WAV endpoint, corrects the first combat encode ending about 0.259 seconds before its audio. Original failed output and diagnostics remain preserved privately. This offline assembly correction changes no gameplay source or packaged runtime; the package remains the verified S3 build without a rebuild.

## Verification evidence

All 15 final S3 packaged runs completed with exit 0, 4,084 assertions and zero
failures. Every runner records successful source/runtime checks before and
after execution. The retained S1 runs below are outside that S3 total.

| Evidence | Result and scope |
| --- | --- |
| Clean public-source Editor/Game/package build at S3 | PASS, all three exit 0; UE 5.7.2, Win64 Development. [Build record](../Evidence/Candidate06/Verification/fresh_build.json) |
| S3 public source payloads and engine automation | PASS: 1,223 tracked files; 660 LFS payloads / 300,752,452 bytes; all 28 tests pass with five warning events. Final D checkout/LFS: The [post-publication verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json) records the final tag/branch/main identity, fresh D checkout and source/evidence LFS verification when issued; no final D result is asserted here |
| S3 runtime identity and scoped privacy audit | PASS: six required runtime identities match the build; 45 retained files / 666,721,590 bytes; no actionable findings. [Runtime audit in the fresh build record](../Evidence/Candidate06/Verification/fresh_build.json) |
| Release archive | PASS: 409,889,719 bytes; 1,217 entries; SHA256 `4dd954cdeb894d2be371fa78df0f640ef7a623685c40aa5cf0261c804f1c8b0a`; all entry CRCs, 1,215 manifest payloads and six extracted runtime identities verified. [local archive/extraction record](../Evidence/Candidate06/Verification/archive.json) |
| Final packaged combat, grouping, survival and relocated map | S3 PASS: 2,110 / 922 / 100 / 35 assertions respectively, all zero failures and exit 0. [All 15 source-bound results](../Evidence/Candidate06/Verification/packaged_checks.json) |
| Retained S1 progression and projected aim | PASS at S1: 95 progression assertions; 2,708 projected-aim assertions / 480 shots; zero failures. [Source comparison and limits](../Evidence/Candidate06/Verification/retained_coverage.md). These runs were not rerun at S3. |
| Final S3 cadence and rendered UI | [Weapon timing](../Evidence/Candidate06/Verification/weapon_timing.md): 197 checks per 30/60/120 cap, 591 total; UI: 49 checks at each of 1600×900 and 1280×720, all zero failures. [UI originals and review](../Evidence/Candidate06/UI/review.md) |
| Final capture stills and UI images | Bounded S3 combat, grouping, survival and portability reviews complete with stated limits; all 12 original UI PNGs inspected. Exact scopes are in the pass. |
| Encoded media | Four S3 recordings pass 24 inspection checks each, including full video/audio decode with exit 0. They contain H.264 1600×900 at 30 FPS and AAC 48 kHz stereo, with separately measured source-WAV, video, audio and container durations. This is byte/format/decode verification; continuous playback and perceptual audio audition were not performed. [media, audio measurements and limits](../Evidence/Candidate06/Media/README.md) |
| Actual engine audio and perceptual listening | Original 48 kHz stereo 16-bit PCM measured; no full-scale samples in any of the four WAVs. AAC decode passes; no perceptual audition or sample-for-sample lossy-codec equality claim. [media, audio measurements and limits](../Evidence/Candidate06/Media/README.md) |
| Native desktop input | [LIMITED PASS](../Evidence/Candidate06/Verification/native_input.json): 14 discrete actions / 22 input edges; normal quit; runtime identities unchanged. No held combinations, movement/approach or audio-audition claim. |
| Recording-free M4A1/870 × requested 6/12/18 profiles | All six executions PASS, 38 assertions each. Measurements/occupancy/interpretation: 22,129 retained frames; means 8.3359–8.3750 ms, p99 8.9514–10.2795 ms and maxima 20.6116–31.4236 ms. Seven frames exceed 16.7 ms and none exceed 33.3 ms. One non-isolated sample per workload supplies no matched-baseline or universal-FPS claim. [full timelines, occupancy and host limits](../Evidence/Candidate06/Performance/README.md) |
| Public downloads and previous-candidate preservation | [Release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate06) / [final download-verification record](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json). Final verification is issued after publication; this committed text does not claim those later checks already ran |

Retained source S1 is `8b5f32640e1239f7035be4c7c1cc6713f0d8be8e`.
The exact S1/S3 comparison establishes identical committed bytes for 119 of
121 source files, including both retained drivers and their production authority.
It does not establish S3 runtime, visual, audio, native-input or performance results.

The [S3 source review](../Evidence/Candidate06/Verification/source_review.md)
reports no actionable finding in the three-file orientation correction.
Input and pickup authority are unchanged; static geometry supports the motif
correction under the ordinary camera. The separate native report records visible
upright skull, gold ×2, teal crate and local glows. All scheduled package runs, six-profile analysis and encoded-media verification are complete.
Neither source review nor the completed build substitutes for the native check.

Aim uses the selected horizontal plane without enemy snapping. Each bullet or
pellet keeps one direction and range budget through bounded body penetration.
Head damage is 1.5×; qualifying head kills add 120 points instead of the normal
100, alongside 10 impact points per living victim per discharge. Regeneration
starts after 15 accepted-damage-free seconds at 10% of current maximum HP/s.

The Box excludes owned families and costs 950. Pack-a-Punch uses a fresh F tap,
accepts reload/empty/pump states and costs 5,000. Processing remains nine seconds.
Ready upgrades return automatically within clear front/side reach to the same
slot, independently of the other gun's reload. The 15-second ready deadline
causes permanent loss without refund; reachable exact-boundary collection wins.

Insta-Kill, Double Points and Max Ammo share one 1% natural drop roll with equal
weights. Timed effects and uncollected drops last 30 seconds; the actor cap is
8. Camera shake has 100%, 50% and Off settings and does not affect logical aim.
Player walk/sprint remain 225/370 cm/s, infected movement 100/195 cm/s, strike
damage 19 and protection 0.55 seconds. Two slots and the M1911 7/56 start remain.

This remains a development sandbox with provisional tuning. Finite still-image samples, numerical fixtures and one non-isolated profile per workload do not establish uninterrupted human play, a universal frame-rate guarantee or player approval. Native held firing combinations, movement feel and perceptual audio audition were not tested. The new pickup cues are synthetic; the accepted infected visual/physical and recorded-audio milestone remains future work.

## Preserved baseline and next milestone

[Candidate05](Passes/Candidate05.md) remains preserved at public revision
`77e182db7d2b4f4e8aba970b0949b9567c94f43a`, packaged from
`6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Its release, earlier candidates and
historical evidence are retained separately; Candidate06 does not rewrite them.

Recommend the accepted [infected redesign and recorded-audio milestone](Roadmap.md):
decayed human visuals, natural pursuit/attacks, controlled crowd physics and
suitable recorded physical sounds. Six perk chambers follow afterward.
Future maps retain hidden spawn → occluded approach → visible entrance → pursuit;
the small Portability06 map is a dependency check, not a finished production map.
