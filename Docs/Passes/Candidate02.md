# Candidate02 — two-weapon combat pass

Candidate02 adds one original pump shotgun to the accepted containment arena,
with carried-weapon switching, distinct reload operations, original weapon
animation/audio and a developer sandbox. It retains the existing player, infected
archetype, room, rounds, points, blood and severing. No Project Zero content,
external asset pack, extra weapon or broader progression system was introduced.

## Release record

[Public repository](https://github.com/laberteauxjacob-cpu/ProjectONE) ·
[Candidate02 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02) ·
[Changes from Candidate01](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate01...candidate02)

| Field | Record |
| --- | --- |
| Public Candidate01 baseline | `02c590a75141b48564715d130018f0cbe9a8a092` |
| Public references | `main` for latest; `codex/candidate02` review branch; `candidate02` version tag |
| Packaged source revision | `0637288d32b6fbebc67ef93d4f03e439ff38bb67` |
| Windows archive | `ProjectONE-Candidate02-Windows.zip` |
| Archive SHA256 | `0cb433a407c46e511c9fc831d516dfbfa68e660bad8c505a3786f04f5e54e7a3` |
| Archive bytes / entries | 397,321,284 / 1,217 |
| Final packaged revalidation | All seven modes passed; 39 weapon checks, including the 13-enemy default-spawn approach regression, reported zero failures. |
| Direct manual follow-up | Passed on `0637288`: F1–F6 retained normal lighting; floor spawns approached; weapon fire/reload/pause/switch, refill/reset and quit checks passed within the short control review. |

The build, runtime suite, performance and capture evidence below belong to
`0637288`. The final repository commit also adds documentation, evidence and
release/validation/capture tooling; `Source`, `Config` and `Content` remain
identical to that packaged revision. The original Candidate01 history bundle,
local rollback tag and package remain preserved separately; private original
history was not included in the public baseline.

## Resulting behavior

AR-01 retains automatic fire, 24/192 ammunition and a 2.10-second magazine reload.
Its magazine is removed at 0.40 seconds, ammunition transfers at 1.20, and the bolt
action occurs at 1.74. SG-01 holds 6/36 shells and fires eight pellets per press.
Its 0.22-second recoil is followed by a 0.56-second pump; the support hand and
fore-end share a travel curve. Shell loading opens for 0.35 seconds, inserts one
earned shell at 0.60 seconds of each 0.90-second cycle, and closes in 0.32 seconds.

Each slot retains ammunition and unfinished pump obligations across a 0.36-second
switch. Interrupted reloads preserve only completed insertions; a short fire tap
can close shell loading and spend an earned shell. Pause, death, refill, reset
and switching clear or suspend the appropriate pending operations. Pellet hits
aggregate once per victim/discharge, avoiding duplicate damage, severing and
points. Surviving arm loss retains the remaining-arm attack.

Sandbox controls expose spawning, refill, reset, cleanup and distance references.
HUD counts and progress read actual gameplay state. Editable definitions, retained
lower-body locomotion, five new static meshes, nine new skeletal clips and 25
original synthesized sounds support this behavior. See
[CurrentState](../CurrentState.md) and the
[asset inventory](../../ArtSource/Weapons/Candidate02/README.md).

## Evidence and its limits

| Evidence type | Recorded status |
| --- | --- |
| Native automation | [Three tests passed on the final source revision](../../Evidence/Candidate02/native_tests.json), covering monotonic damage/death, regional trauma boundaries and packet idempotence. Two reported known pre-BeginPlay mesh-query warnings; none failed. Native tests are separate from the packaged integration runs. |
| Packaged runtime checks | All seven final modes passed: [39 weapon checks](../../Evidence/Candidate02/combat_checks.txt), [retained-system checks](../../Evidence/Candidate02/legacy_combat_checks.txt), [presentation checks](../../Evidence/Candidate02/presentation_checks.txt), [comparison checks](../../Evidence/Candidate02/comparison_checks.txt) and the three benchmark scenarios below. They exercise real gameplay objects, not weapon/damage mocks. |
| Source and imports | [All 78 source/import hashes match](../../Evidence/Candidate02/source_sync.json); [five meshes, nine clips and 25 sounds imported](../../Evidence/Candidate02/WeaponAssetImport.json). Source motion roundtrips and metadata-only import-record updates have separate reports. |
| Separate public checkout | The verified public clone was cleanly fast-forwarded to `0637288`: [202 LFS files totaling 78,275,144 bytes](../../Evidence/Candidate02/fresh_build.json) hydrated with matching hashes/sizes. Editor, game and package builds exited zero; tracked source stayed unchanged and private original history was absent. This used the existing validation host, not a clean Windows installation. |
| Separate performance runs | [6](../../Evidence/Candidate02/benchmark_6.txt), [12](../../Evidence/Candidate02/benchmark_12.txt) and [18](../../Evidence/Candidate02/benchmark_18.txt) sustained live infected: mean 8.385/8.387/8.388 ms, all median/p95 8.334 ms. RTX 3090, Ryzen 9 5950X, 64 GB RAM, 1600×900, FXAA, scalability 2 and a 120 FPS cap. These capped measurements do not establish uncapped headroom or lower-end hardware performance. |

The [genuine comparison capture](../../Evidence/Candidate02/WeaponComparison.mp4)
contains **470 captured frames at 22.38638 actual frames/second**. Its 30 FPS encoded
output holds timestamped frames as needed; it does not create extra captured
motion. The master recording is 20.97066667 seconds of 48 kHz stereo PCM. The
[audio measurements](../../Evidence/Candidate02/audio_metrics.json) report
−13.019964 dBFS sample peak, −37.584901 dBFS RMS, zero full-scale samples and complete,
non-silent requested weapon phases. An earlier background recording was silent
and rejected. The accepted capture uses a process-only
`-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0` launch override;
no gameplay source or packaged configuration changed for it. Audio-input tooling
could not provide perceptual audition. These numbers do not establish convincing timbre, mix quality or
sound/animation synchrony.

Each benchmark sampled about 20 seconds after a 10-second warmup, with 2,386 /
2,385 / 2,384 frame samples respectively. No continuous capture or build ran
during those measurements; each benchmark took one screenshot. The recorded
mean/median/p95 do not quantify startup or rare worst-frame hitches.

The separate [13-frame visual review](../../Evidence/Candidate02/visual_review.md)
inspected exact timestamps during both reloads and the shotgun pump. It found
corresponding hand/fore-end positions and changing leg poses during pumping, with
no gross weapon separation or limb collapse in those samples. Dark magazine/hand
surfaces and the tiny shell limit contact readability. Stationary reload samples
do not verify moving reloads; stills cannot establish smoothness or rule out foot
sliding. It explicitly records the capture build revision and all 13 timestamps.

[Direct manual input review](../../Evidence/Candidate02/manual_playtest.md) is a
third category. The initial short review verified normal launch, death/restart,
weapon selection, fire/reload/pause, refill/reset and quit. It also found two
F2/F3-spawned infected stranded on rack navigation islands despite the earlier
automated passes. A targeted correction searches up to 17 nearby positions on the player's floor,
requires a complete nonpartial navigation route and capsule clearance, and avoids
post-check collision relocation. A new regression follows the actual F2/F3/F3
sequence: all 13 accepted enemies had complete routes, remained on the floor and
approached over eight seconds in the final packaged check. Direct manual
F2 follow-up also confirmed a floor spawn reaching and attacking the player.
That review exposed a separate inherited-debug-key conflict: F2 changed the
view to unlit and F5 enabled shader complexity. Final input configuration removes
only those five conflicting bindings. The final `0637288` direct review passed:
F1–F6 retained normal lighting, F2/F3 spawns stayed on the floor, and weapon
selection, firing, reload/pause, wheel switching, refill/reset and quit behaved as
recorded. Exact cleanup counts remain an automated check; this was a short
control review, not a sustained human playthrough or a clean-machine install test.

The final [extracted-archive startup check](../../Evidence/Candidate02/release_smoke.json)
verified every manifest hash/size and launched the root executable without flags.
The arena and ordinary round rendered, but a Windows firewall prompt appeared.
No security-prompt action was taken; the smoke-test process was stopped. The
separate control review used the identical adopted package.

## Reproduction, privacy and next focus

Build metadata records UE 5.7.2, MSVC 14.50.35725, Windows SDK 10.0.26100.0,
Win64 Development. The compiler is newer than UE's preferred 14.44 version.
Git LFS carries runtime Content, Blender sources, FBXs, WAVs and video. Private
author paths in asset metadata were removed with semantic/pixel preservation
checks; old import hashes were refreshed without reimporting art. Archive
integrity/privacy results are [recorded separately](../../Evidence/Candidate02/release_audit.json)
and passed for the final checksum, including archive CRC, manifest hashes and
byte-identical fresh runtime files. See [DistributionNotice](../DistributionNotice.md)
for runtime notices and ownership; no broad license was selected for original
code or art.

The required source and editable assets are published through Git LFS; normal
uploads and fetches were verified. No paid settings were enabled. The current
authentication did not expose account-specific LFS quota or budget, so this
record does not assert an available allowance.

The next focused recommendation is a weapon-feedback review with actual human listening and normal-camera reload
readability checks. Improve specific timbre/contact problems identified there
before adding content. Character polish and synthesized audio remain provisional;
this pass does not claim final visual or sonic realism.
