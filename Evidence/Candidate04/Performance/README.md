# Candidate04 machine/combat performance

These three completed, media-free packaged profiles use source `8055041ebc98a4df7cd8923b05e7b89ad7372e38`.
Resolution is 1600x900 and the runner requests cap120 / VSync0. Each run records the actual CSV resolution and available console readbacks separately.

**Different workload:** Candidate04 includes two operating machines, actual carried Last Word fire/reloads, production-input movement, a replenished live-count target, physical corpses and profile-only health protection. The machine processes/displays M4 to Overcurrent; carried Overcurrent and Gravebreaker firing are not profiled. Candidate03 is the preserved stationary living-only reference. Its benchmark screenshot spike remains in its data; Candidate04 has no media capture. No causal improvement or GPU-cost claim is made.

Full captures retain every numeric frame, including the initial spawn/CSV transition row and all spikes. Initial machine construction, box acquisition and the actual pistol-upgrade setup occurred before CSV and are not measured. Setup combat can leave a real corpse/blood or survivor. The 10–20-second view selects complete frames on the cumulative CSV FrameTime clock; simultaneous-machine results instead select their actual Active counters.

## Frame times

Cells show Candidate03 → Candidate04 (difference), in milliseconds.

| Count / selection | Mean | p95 | p99 | Maximum |
| --- | --- | --- | --- | --- |
| 6 / full | 8.3754 → 8.3350 (-0.0404) | 8.5588 → 8.8103 (+0.2515) | 8.6977 → 9.4036 (+0.7059) | 132.6865 → 12.5948 (-120.0917) |
| 6 / 10to20 | 8.4330 → 8.3350 (-0.0980) | 8.5343 → 8.8477 (+0.3134) | 8.6609 → 9.3771 (+0.7162) | 132.6865 → 10.1841 (-122.5024) |
| 12 / full | 8.3818 → 8.3369 (-0.0449) | 8.6061 → 8.8281 (+0.2220) | 8.7725 → 9.3608 (+0.5883) | 136.6046 → 17.1668 (-119.4378) |
| 12 / 10to20 | 8.4376 → 8.3361 (-0.1015) | 8.6000 → 8.8405 (+0.2405) | 8.7235 → 9.4126 (+0.6890) | 136.6046 → 10.2626 (-126.3420) |
| 18 / full | 8.3861 → 8.4123 (+0.0262) | 8.8042 → 9.1053 (+0.3011) | 9.0725 → 9.7091 (+0.6366) | 140.2602 → 22.0846 (-118.1756) |
| 18 / 10to20 | 8.4411 → 8.3399 (-0.1012) | 8.8229 → 8.9109 (+0.0880) | 9.0602 → 9.3580 (+0.2978) | 140.2602 → 10.2043 (-130.0559) |

Threshold counts use strictly greater than 16.7, 33.3, 50 and 100 ms; no long frames are removed.

| Requested live | Full rows / actual counter samples | Exact-count fraction | Both Active seconds | Full spikes >16.7 /33.3 /50 /100 ms |
| --- | --- | --- | --- | --- |
| 6 | 3766 / 3765 | 75.01% | 3.700900 | 0 / 0 / 0 / 0 |
| 12 | 3766 / 3765 | 71.42% | 3.700400 | 1 / 0 / 0 / 0 |
| 18 | 3733 / 3732 | 78.40% | 3.700200 | 1 / 0 / 0 / 0 |

Actual live-count histograms are in summary.json. Zero/unregistered initial counter rows remain in the full timeline and are marked invalid for actor-count reconciliation.

## Individual machine CPU scopes

`MachineState`, `MachineVisual` and `PresentationDriver` are regular **inclusive** scopes. MachineVisual is nested inside MachineState on ordinary ticks. They must not be added or subtracted to estimate unique CPU time. Means include frames in which a scope did not execute.

| Count / scope | Full mean / p99 / max ms | Both Active mean / p99 / max ms |
| --- | --- | --- |
| 6 / MachineState | 0.026822 / 0.060475 / 0.176400 | 0.039759 / 0.096799 / 0.117200 |
| 6 / MachineVisual | 0.025550 / 0.045205 / 0.129000 | 0.036390 / 0.059327 / 0.072600 |
| 6 / PresentationDriver | 0.047637 / 0.130650 / 4.878300 | 0.049650 / 0.127430 / 0.161000 |
| 12 / MachineState | 0.028175 / 0.056915 / 0.285900 | 0.039619 / 0.084054 / 0.108400 |
| 12 / MachineVisual | 0.026888 / 0.041235 / 0.284900 | 0.036361 / 0.047269 / 0.062700 |
| 12 / PresentationDriver | 0.048714 / 0.082095 / 8.865700 | 0.049958 / 0.089953 / 0.159900 |
| 18 / MachineState | 0.029299 / 0.084680 / 0.374400 | 0.039443 / 0.086014 / 0.176500 |
| 18 / MachineVisual | 0.027910 / 0.057572 / 0.372600 | 0.036201 / 0.052126 / 0.175800 |
| 18 / PresentationDriver | 0.054747 / 0.153640 / 13.325600 | 0.048874 / 0.087216 / 0.144100 |

Individual PhysicsVerbose/Chaos/ONEPhysicality CPU scopes remain in each report under their actual names. Nested solver timings and worker aggregates overlap; no combined physics wall-time total is inferred.

## Provenance and limits

Original CSV filename, SHA256, byte count, all row counts, begin/end markers and actor-report reconciliation are recorded without publishing event text, command lines, hostname, user or absolute paths. Each of the three complete numeric timelines is reparsed and compared against every numeric input cell before its SHA256 is inventoried. Selected reports reference exact original frame-index ranges in the one full timeline.

There is one run per count. Background application, power/foreground state and different screenshot workloads limit comparison. Ambient-app evidence is recorded only when supplied and verified by the runner; identical background activity is never inferred.

The private curator produced these review artifacts after all three runs completed. See summary.json for runtime identity and settings evidence, comparison.json for full precision and inventory.json for output byte identities.
