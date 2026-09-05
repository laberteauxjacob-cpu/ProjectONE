# Candidate03 measured performance

The final fresh Candidate03 package completed all four profiling scenarios. Living-crowd average frame times stayed close to the 120 FPS cap, while individual solver CPU costs increased. At 18 enemies, the primary comparison's p99 rose from 8.793 to 9.060 ms (+0.267 ms). The separate 117.060-second physicality run recorded five frames above 16.7 ms, a 31.426 ms maximum and none above 33.3 ms. These are one-run measurements on the validation machine, not uncapped capacity or a visual-quality verdict.

Candidate03's built source is [`e81e1137ed891570c73c8c278fc2c8cc2250bd04`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/e81e1137ed891570c73c8c278fc2c8cc2250bd04). The preserved Candidate02 package was built from [`0637288d32b6fbebc67ef93d4f03e439ff38bb67`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/0637288d32b6fbebc67ef93d4f03e439ff38bb67). Source/runtime hashes, successful exits, completed CSV footers and same-run scenario assertions were verified before curation. See the [C03 summary](C03/summary.json), [comparison](C03/comparison.json) and [preserved C02 summary](C02/summary.json).

## Conditions and selection

Both builds used Windows Development, UE 5.7.2, Ryzen 9 5950X, RTX 3090, 1600×900, VSync off and a 120 FPS cap. Recorded view-distance, anti-aliasing, shadow, post-process, texture and effects quality levels were 2; screen percentage was 100, anti-aliasing method 1, dynamic global illumination 0 and reflection method 0. The living runs' end reports verify those settings and actual living counts of 6, 12 and 18. This is an end-count assertion, not independently sampled per-frame population data.

Each living capture contains 3,000 frames. The predeclared primary selection contains complete frames inside **CSV-relative 10–20 seconds**, using cumulative FrameTime, without exclusions. It is not exact actor/world time. Candidate03's logged CSV-start offsets from validation begin were 0.210/0.244/0.227 seconds for 6/12/18; no invented time correction is applied. Full results preserve startup/spawn work. Each build's existing benchmark PNG at actor elapsed 15 seconds remains in both the full and selected data. Every selected living run contains one frame over 100 ms; these screenshot-associated spikes were not removed.

Candidate03 uses bounded physics substeps (maximum 1/120 second, six substeps, 0.05-second maximum physics delta), revised animation, regional queries, collision and navigation. These system changes and the ambient-app limitation below prevent attributing every difference to one change. The physicality actor has startup/config settings evidence and resolution readback, but no end-of-run ScreenPercentage readback.

## Living crowd: primary 10–20-second window

Cells show **C02 → C03 (C03 minus C02)**, all in milliseconds. Counts are 1,184/1,184/1,183 selected frames in each build for 6/12/18 enemies. Lower time is better; one capped run does not establish a meaningful improvement from tiny average differences.

| Living enemies | Mean | p95 | p99 | Maximum |
| --- | --- | --- | --- | --- |
| 6 | 8.4356 → 8.4330 (−0.0026) | 8.5147 → 8.5343 (+0.0196) | 8.6117 → 8.6609 (+0.0492) | 135.5696 → 132.6865 (−2.8831) |
| 12 | 8.4410 → 8.4376 (−0.0034) | 8.5642 → 8.6000 (+0.0358) | 8.7093 → 8.7235 (+0.0142) | 141.3121 → 136.6046 (−4.7075) |
| 18 | 8.4418 → 8.4411 (−0.0007) | 8.6009 → 8.8229 (+0.2220) | 8.7933 → 9.0602 (+0.2669) | 142.3466 → 140.2602 (−2.0864) |

The p99 differences are +0.57%, +0.16% and +3.03% respectively. Each side has exactly one selected frame above each threshold: 16.7, 33.3, 50 and 100 ms. Full precision, thread timings and individual physics scopes are in [6](C03/alive6_10to20.json), [12](C03/alive12_10to20.json) and [18](C03/alive18_10to20.json).

## Living crowd: complete captures

These results retain all 3,000 frames, approximately 25.1 seconds per run. Cells retain the same C02 → C03 (delta) convention in milliseconds.

| Living enemies | Mean | p99 | Maximum | Frames >16.7 ms, C02 → C03 |
| --- | --- | --- | --- | --- |
| 6 | 8.3737 → 8.3754 (+0.0017) | 8.6419 → 8.6977 (+0.0558) | 135.5696 → 132.6865 (−2.8831) | 2 → 3 |
| 12 | 8.3764 → 8.3818 (+0.0054) | 8.7180 → 8.7725 (+0.0545) | 141.3121 → 136.6046 (−4.7075) | 2 → 3 |
| 18 | 8.3818 → 8.3861 (+0.0043) | 8.7907 → 9.0725 (+0.2818) | 142.3466 → 140.2602 (−2.0864) | 3 → 3 |

All six full captures have one frame above 33.3 ms, also above 50 and 100 ms. Full reports: [6](C03/alive6_full.json), [12](C03/alive12_full.json), [18](C03/alive18_full.json).

## Individual physics CPU scopes

The following are recorded `PhysicsVerbose/AllWorkers` scope durations in the primary window. They include nested scopes and worker aggregates: **do not add them together or interpret their sum as physics wall time**. A small capped frame-time change does not mean unchanged CPU work.

| Scope / living count | Mean C02 → C03, ms | Mean delta, ms | p99 C02 → C03, ms |
| --- | --- | --- | --- |
| StepSolver / 6 | 0.1532 → 0.2535 | +0.1003 | 0.1942 → 0.3494 |
| StepSolver / 12 | 0.1801 → 0.3211 | +0.1411 | 0.2538 → 0.4383 |
| StepSolver / 18 | 0.2081 → 0.4351 | +0.2271 | 0.3066 → 0.5891 |
| StepSolver_AdvanceOneTimeStepImpl / 6 | 0.0852 → 0.1430 | +0.0578 | 0.1106 → 0.2033 |
| StepSolver_AdvanceOneTimeStepImpl / 12 | 0.0899 → 0.1656 | +0.0757 | 0.1292 → 0.2373 |
| StepSolver_AdvanceOneTimeStepImpl / 18 | 0.0974 → 0.2117 | +0.1142 | 0.1555 → 0.2858 |
| StepSolver_DetectCollisions / 18 | 0.0109 → 0.0178 | +0.0069 | 0.0171 → 0.0267 |

## Physicality and mixed stress workload

This is a separate Candidate03-only scenario; Candidate02 has no equivalent ragdoll/wound workload. The actor owned CSV start/stop after warmup, finished all 11 phases and waited for the writer. Its own run passed **257 assertions, zero failures, zero captured media frames**. It is distinct from the 249-assertion rendered recording and earlier 244-check editor capture. No JPEG, video or audio recording ran during this profile, and detailed per-corpse diagnostic sampling was disabled.

The full capture contains **14,038 frames over 117.0604 seconds**, mean 8.3388 ms, p95 8.6937 ms, p99 9.0456 ms and maximum 31.4258 ms. Five frames exceeded 16.7 ms; none exceeded 33.3/50/100 ms. All initialization, fixture cleanup and transition frames remain. See the [full report](C03/physicality_full.json).

| Recorded phase | Frames / seconds | Mean / p99 / max, ms | Frames >16.7 ms | StepSolver mean / p99 / max, ms |
| --- | --- | --- | --- | --- |
| [5: six moving deaths and corpse contact](C03/physicality_phase05.json) | 1,560 / 13.013 | 8.3414 / 8.7981 / 23.3295 | 1 | 0.8828 / 1.3680 / 1.7572 |
| [6: corner crowd](C03/physicality_phase06.json) | 1,320 / 11.019 | 8.3477 / 8.9647 / 31.1458 | 1 | 0.4239 / 0.5719 / 2.8539 |
| [7: rack crowd](C03/physicality_phase07.json) | 1,319 / 11.010 | 8.3474 / 8.8608 / 31.4258 | 1 | 0.4310 / 0.6012 / 3.3092 |
| [8: bench-end crowd](C03/physicality_phase08.json) | 1,319 / 11.001 | 8.3406 / 9.0451 / 22.1838 | 1 | 0.4446 / 0.5988 / 3.4295 |
| [9: mixed living/dead encounter](C03/physicality_phase09.json) | 2,399 / 20.009 | 8.3407 / 9.5204 / 21.4826 | 1 | 1.5650 / 2.2371 / 3.1371 |

Phase 5 reached 96 simulated bodies; its final recorded count was 48 simulated, 16 awake. Frozen kinematic bodies retain collision and are excluded from that simulated count. Phase 9 reached 18 living enemies, 208 simulated body instances including parts, 160 awake instances and 13 wounds. Its maximum projection count was 12. Native Chaos body counters can aggregate different solver/substep samples and are not interchangeable with these project counts.

For phase 5, `StepSolver_AdvanceOneTimeStepImpl` was mean/p99/max 0.7053/1.1177/1.4651 ms; `StepSolver_DetectCollisions` was 0.2393/0.3679/0.4533 ms. In mixed phase 9 these were 1.1370/1.5916/2.2019 ms and 0.3372/0.5305/0.7235 ms respectively. They remain separate overlapping measurements.

Project CPU scopes below are `ONEPhysicality/GameThread`. Means and percentiles include frames where the scope did not execute; “nonzero frames” is not a call count or a single-operation latency.

| Scope | Full mean, ms | Full p99, ms | Full maximum, ms | Nonzero frames |
| --- | --- | --- | --- | --- |
| RagdollRest | 0.001707 | 0.0274 | 0.3240 | 1,752 |
| RagdollInitialize | 0.002206 | 0 | 5.0023 | 43 |
| StumpCollisionTransfer | 0.000021 | 0 | 0.0438 | 43 |
| PartInitialize | 0.000311 | 0 | 0.6536 | 9 |
| BloodFixedStep | 0.003166 | 0.0895 | 0.4592 | 753 |

The five longest frames are indices 9960, 8641, 7321, 11279 and 13678, at CSV-relative 83.0337, 72.0237, 61.0125, 94.0441 and 114.0543 seconds. They coincide with phase transitions; the first two include 4.9641 and 5.0023 ms of RagdollInitialize respectively. Fixture cleanup retires living subjects through production death before preparing the next phase, so those costs are retained rather than presented as ordinary steady pursuit. Phase reports use the recorded counter bounded by original begin/end events; a boundary frame can contain the next fixture's setup. This observation does not attribute the entire long frame to ragdoll initialization.

## Ambient application and limits

An existing unsaved, interactive Blender window remained open and untouched under an explicit exception. Its process identity and creation time were bound privately; command-line arguments were disallowed. At least five seconds of preflight CPU sampling preceded each scenario. The fixed limit was 0.05 CPU-seconds per wall-second, relative to one logical CPU. Before/after accounting verified the same process throughout each scenario; no measurement exceeded the limit.

| Scenario | Blender CPU seconds / accounting wall seconds | CPU-seconds per wall-second |
| --- | --- | --- |
| Living 6 | 0.312500 / 35.516 | 0.008799 |
| Living 12 | 0.328125 / 35.547 | 0.009231 |
| Living 18 | 0.234375 / 35.454 | 0.006611 |
| Physicality | 0.859375 / 129.562 | 0.006633 |

These intervals include process launch/shutdown and are not a continuous CPU monitor; short bursts cannot be excluded. Candidate02 ambient CPU was not recorded this way, so identical background activity is not claimed. PID, window title and private paths are omitted. The runner detected no build, import, encoding, OBS or second engine process at scenario boundaries. Unsampled background activity, power state and foreground differences remain possible. There was one repeat per build/count; no confidence interval or causal performance claim is warranted.

## Reproducible evidence

[C03/inventory.json](C03/inventory.json) binds 18 full/window/phase analysis reports, the comparison and summary, and **only four full numeric timelines**. Each selected report references its full CSV hash and exact inclusive original frame-index ranges. This preserves spike data once without redundant derived CSV copies. Full precision, individual solver scopes and counters remain available. Raw engine logs, CSV event text, commands and private footer metadata remain outside Git. The [public analyzer](../../../Scripts/analyze_candidate03_performance.py) documents the percentile calculation and complete-frame selection rule. Captured source/runtime identities, counts, settings and privacy were checked; no measured frame was altered or dropped to improve the result.
