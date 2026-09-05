# Candidate04 evidence

Candidate04 adds the M1911 starter, Mystery Box, Pack-a-Punch, two owned slots
and three independent upgrades. The verified gameplay source is
[`8055041ebc98a4df7cd8923b05e7b89ad7372e38`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/8055041ebc98a4df7cd8923b05e7b89ad7372e38).
The public checkout built successfully, all nine native tests passed, and the
exact package passed all 15 modes with 984 assertions and zero failures.

The implementation and exact controls are documented in the
[pass report](../../Docs/Passes/Candidate04.md). Source-generation checks,
native tests, production-input dispatch, chronological frame inspection,
engine audio measurements, native desktop input and packaged testing remain
separate evidence categories. Numerical audio measurements do not establish
perceptual listening or mix approval.

[AudioImport.json](AudioImport.json) records the targeted import of the 28 new
weapon SoundWaves. Machine and weapon source inventories are retained alongside
their editable assets. Dense captures, raw engine logs and private diagnostics
remain outside source history; selected review movies and portable verification
records are listed below. Raw captures are not public release payloads.

## Build, source and packaged checks

[Fresh build](Verification/fresh_build.json) records UE 5.7.2 Editor, game and
Development packaging, 875 tracked files, 488 verified LFS payloads totaling
233,654,563 bytes, exact native test names and six runtime hashes. Six native
tests were clean; three produced five isolated test-world teardown warnings.
All four inventory tests are warning-free. The build used a clean public checkout
on an existing development host and an incremental public fast-forward, not a
clean operating-system installation or cleared build caches.

[Source privacy](Verification/source_privacy.json) states the earlier full
working-payload audit and final 318-file text audit separately, including the
two public source commits. It does not claim a final full binary rescan.

[Packaged suite](Verification/packaged_suite.json) verifies each mode's own
completion marker, exit code 0, failed assertion count and exact fresh
runtime identity. C04 actor reports match their complete same-run assertion
streams. All six effective weapons are checked, and all 54 legacy movement
trials verify their requested slot and family before acting.

| Packaged mode | Passed assertions |
| --- | ---: |
| ONE03MovementCheck | 112 |
| ONE03WeaponCheck | 85 |
| ONE03CaseCheck | 49 |
| ONE03PresentationCheck | 121 |
| ONE03DamageCheck | 42 |
| ONE03PhysicalityCheck | 241 |
| ONE04ProgressionCheck | 72 |
| ONE04ArsenalCheck | 148 |
| ONECombatCheck | 40 |
| ONECompare | 3 |
| ONEPresentation | 33 |
| ONEValidate | 35 |
| ONEBenchmark=6 | 1 |
| ONEBenchmark=12 | 1 |
| ONEBenchmark=18 | 1 |
| Total | 984 |

Benchmark assertions confirm the requested actual living counts. These default
suite modes do not establish performance: some record images/audio, and each
legacy benchmark retains one screenshot. The suite report discloses high-health
targets, direct regional packets, extended debris-cap lifetimes and other
fixtures. Earlier native float-tolerance and legacy-loadout failures remain
described in the pass report; their initial results are not substituted for
the verified reruns.

## Recorded presentation and native launch

The final packaged walkthrough passed 58 functional assertions and produced
3,001 original frames with 148.11733333 seconds of genuine engine WAV. The
earlier 2,932-frame attempt also passed 58 assertions but failed strict assembly
with a 34.454 ms callback overrun. The final capture's overrun was 9.795667 ms.
[Assembly tooling](Tools/README.md) preserves all raw identities and explicitly
omits only `frame_03000.jpg`, an unsupported callback in the final idle chapter.
The presented 3,000 callbacks retain 2.956479 seconds of final idle and the full
original audio duration. The raw WAV is unchanged; there is no padding,
stretching or removed gameplay event.

[Visual review](Verification/visual_review.json) binds 75 inspected original
frames across both captures: 68 dense and seven sparse. It supports readable
handoff/retrieval, machine forms and case flight, with unresolved fine fingers,
shell seating, slide and fore-end contact. Missing HUD glyphs remain visible in
JPEGs; a capture-only cause has not been established. This is a bounded original
frame review, not full-motion playback or unrestricted visual approval.

[Audio metrics](Verification/audio_metrics.json) cover all 23 selected action
phases with signal, a -14.632356 dBFS overall peak, -41.983174 dBFS RMS and zero
full-scale samples or audit warnings. No perceptual audition is claimed.

| Movie | Duration / chapters | Size / SHA-256 |
| --- | --- | --- |
| [Full progression](progression_loop.mp4), [sidecar](progression_loop.json) | 148.117333 s / 80 | 30,033,564 bytes; `537495bf1692ef74f321a25108799818d05626fcbec731b89d3146093280801b` |
| [Weapon comparison](weapon_comparison.mp4), [sidecar](weapon_comparison.json) | 67.9 s / 8 | 7,949,954 bytes; `e3b4c6e5535ad06d788fec4a13cad965d89b8773d6cbd10901c57b07b1b9947d` |

Both complete audio/video decodes passed. The comparison contains 2,037 video
frames across eight disclosed excerpts, using the actual repaired full movie
and matching original PCM intervals. Its reordered base/upgraded pairs and
dim-machine clips are not continuous gameplay. Sidecars preserve original and
presented frame identities, exact cuts and source bindings.

The [public evidence tools](Tools/README.md) apply the explicit terminal-idle
policy without changing the S2 runtime. Container chapter titles then required
stream-copy repairs in both movies. Every title was verified, encoded audio/video
packet hashes stayed identical and the missing-track warning was absent on final
verification. Original pre-remux encodes remain preserved. No game rebuild,
motion interpolation or audio replacement followed S2; decode checks are not
perceptual playback or audition.

[Native controls](native_controls.json) records ordinary sandbox launch through
`Launch.ps1`, exit code 0, and a native view of M1911 7/56, Empty slot 2 and zero
points from the extracted archive. A Windows Security firewall prompt blocked
gameplay input and was left unchanged for the user. No native gameplay events,
native video or held-input PASS is claimed. Only the two launched game processes
were cleaned up; this does not verify the in-game Quit action.

## Performance and distribution

[Performance](Performance/README.md) contains three completed media-free packaged
profiles with exact source/runtime bindings, full numeric timelines and selected
10–20-second/simultaneous-machine windows. At 1600x900, cap 120 and VSync off,
6/12/18 requested live targets gave mean frame times of
8.3350/8.3369/8.4123 ms and p99 of 9.4036/9.3608/9.7091 ms. Counts were exact
in 75.01/71.42/78.40 percent of valid counter samples; both machines were Active
together for about 3.70 seconds per run. All spikes remain: full maxima were
12.5948/17.1668/22.0846 ms, with no frame over 33.3 ms.

The [summary](Performance/summary.json) and
[comparison](Performance/comparison.json) distinguish moving Last Word combat,
corpses and machine work from Candidate03's stationary living-only reference and
retained screenshot spike. Setup precedes CSV, one run per count limits inference,
and nested CPU scopes must not be summed. No causal improvement or GPU-cost
claim is made. The [inventory](Performance/inventory.json) binds the 15 payloads;
the inventory itself makes 16 curated files.

[Archive verification](Verification/archive.json) passed for
`ProjectONE-Candidate04-Windows.zip`: 1,217 entries, 407,635,789 bytes, SHA-256
`1ad1a3258f16fccf12451fff829aa9124321491a457232b88efa871826ba783c`.
All 45 runtime files remain byte-identical to the audited fresh package, with
all six required files matching the source build. Manifest sizes/hashes, CRCs,
notices and safe entry paths were checked. Separate verified extraction supplied
the native launch package and preserved Candidate03.

The
[candidate04 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate04)
carries the Windows archive and evidence. Its separate
[Candidate04-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate04/Candidate04-PublicationVerification.json)
attachment records checks performed after upload: exact final commit, downloaded
hashes, full outgoing history audit and hydrated public checkout/LFS. Archive
integrity and adoption do not themselves prove a public download. The earlier build
JSON retains its build-time suite-pending field; the completed suite JSON is the
current packaged result. The exact built source is separate from later
documentation/evidence publication commits.
The source-privacy JSON retains its earlier scopes; the attachment records the
final outgoing audit rather than retroactively expanding that historical report.

[Preservation](Verification/preservation.json) confirms six recorded Candidate03
file identities, its unchanged local/public annotated tag and all 15 performance
payload identities. The [Candidate03 evidence](../Candidate03/README.md) and
released archive remain separate. This does not repeat Candidate03 gameplay.
