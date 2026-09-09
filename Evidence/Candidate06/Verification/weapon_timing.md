# Candidate06 packaged weapon timing

Source: `c2c422012f4f219fb5e63ac24294a6ba8428f999`

All three requested caps (30, 60 and 120) passed 591 recorded checks, with zero failures. Every ordered assertion matches its engine log. All six weapon variants completed the eligible-tap, loadout, full-capacity ammunition and reload/dry-fire paths; exact per-variant results and input hashes are retained in `weapon_timing.json`.

| Cap | Weapon | Configured RPM | Full-mag shots | Measured RPM | Mean interval (ms) | Observation gap (frames) | Max absolute phase error (ms) |
| ---: | --- | ---: | ---: | ---: | ---: | --- | ---: |
| 30 | M4A1 | 600 | 24 | 599.997 | 100.000435 | 3 | 0.011978 |
| 30 | Overcurrent | 690 | 36 | 684.779 | 87.619457 | 2, 3 | 31.896356 |
| 60 | M4A1 | 600 | 24 | 599.996 | 100.000652 | 6 | 0.014977 |
| 60 | Overcurrent | 690 | 36 | 688.419 | 87.156257 | 5, 6 | 16.129678 |
| 120 | M4A1 | 600 | 24 | 599.987 | 100.002130 | 12 | 0.048977 |
| 120 | Overcurrent | 690 | 36 | 688.511 | 87.144571 | 10, 11 | 8.022310 |

RPM uses the N−1 intervals between N actual discharges, excluding reload time. Discharge times come from positive shot-counter transitions and `last_shot_time`; one restart reset per run is excluded. JSON also retains the shorter held-burst sections, every measured gap summary and the actual ordinary game-frame intervals. Finite windows and available game frames can make measured RPM differ from the configured 600/690.

Each automatic weapon records one recovery shot after the deliberate 320 ms stall and no catch-up shot in the short guard before key flushing. All six variants pass the same-frame and next-pose late-tap assertions at each cap. These are component scheduling checks, not native-input latency measurements.

The legacy ONE05WeaponCheck name now emits candidate06 checks and requires a genuinely unowned-family acquisition plan before testing ordinary reload rejection. Its passing cases retain committed magazine reload and shotgun shell-return coverage.

This offline summary reads completed artifacts only. Source/runtime linkage comes from the exact-source fresh-build record and verified runner results; raw results, checks, timeline and log hashes are retained. No continuous playback, audio audition, native mouse-to-photon, performance, host-isolation or visual-approval claim is made. The broader driver session may still have other modes pending.
