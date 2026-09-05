# Candidate03 first-source packaged validation

Source `6c7a00c325eb3f4b43b42f186e1d04c2e07fee8f` built successfully, but its packaged suite did **not** pass: nine modes passed, `ONEValidate` recorded 33 PASS assertions and one FAIL, and the three benchmarks were not run. The sequential runner stopped at that failure. The incomplete summary contains only the nine successful process exits; no failed-mode exit value is inferred here. Exact log hashes, byte counts and categorical warnings are in [first_source_validation.json](first_source_validation.json).

| Mode | Outcome | PASS lines | FAIL lines |
|---|---|---:|---:|
| ONE03MovementCheck | PASS | 58 | 0 |
| ONE03WeaponCheck | PASS | 85 | 0 |
| ONE03CaseCheck | PASS | 49 | 0 |
| ONE03PresentationCheck | PASS | 121 | 0 |
| ONE03DamageCheck | PASS | 42 | 0 |
| ONE03PhysicalityCheck | PASS | 245 | 0 |
| ONECombatCheck | PASS | 40 | 0 |
| ONECompare | PASS | 3 | 0 |
| ONEPresentation | PASS | 33 | 0 |
| ONEValidate | FAIL | 33 | 1 |
| ONEBenchmark=6 | NOT_RUN | not run | not run |
| ONEBenchmark=12 | NOT_RUN | not run | not run |
| ONEBenchmark=18 | NOT_RUN | not run | not run |

The failed assertion was **“One-arm infected moves and continues pursuing.”** That packaged log has no trajectory coordinates, so it does not establish the cause by itself.

A separate local rendered editor diagnostic, using the pre-correction source plus telemetry, reproduced the old assertion failure. At stage 2 it recorded a living infected displaced 88.565 cm from spawn and 131.435 cm from a stationary target initially 220 cm away. At stage 7 it remained alive and only 59.120 cm from the moving target, while its net displacement from the original spawn was 35.786 cm, below the old 40 cm threshold. A separate null-RHI diagnostic ended 78.544 cm from spawn and passed the old assertion. These are local diagnostic measurements, **not packaged trajectory measurements**; they show that the later endpoint depends on execution context.

The local fixture now checks early approach at stage 2: alive, anatomical right arm missing, left arm present, travel from spawn greater than 40 cm, and more than 40 cm closer to the stationary target. Stage 7 checks the same survival/arm state and distance to the current moving target below the existing initial-distance-plus-50 cm limit (270 cm here). This separates early movement from later proximity instead of treating late net displacement as total travel. The revised fixture is **pending a fresh new-source package rerun** at the time of this record; no pursuit-runtime fix or final pass is claimed.

Earlier first-source build, native-test and privacy audit successes remain valid historical records. Preserve those records when the final source replaces their primary filenames. This note supplies no new performance, perceptual audio, visual-direction or direct manual approval.


The first local rendered retest of the split fixture passed early approach but failed stage 7 with target distance **240.004 cm < 270.000 cm**. Thus another guard in the combined survival/arm-state assertion failed; the interim label did not contain individual state values. A separate review of that run's `05_combat` screenshot reported an additional corpse, which does not establish the specific guard or shot trajectory here. This retest was local editor-game work, not the original package.

The scripted firing direction still depended on the desktop cursor. A second fixture correction now fixes aim toward pawn position plus `(0,1500,30)` during stages below 8, perpendicular to the +X movement/pursuit arrangement, then clears the override. Pursuit labels now report `alive`, `right`, and `left` explicitly. These are validation-driver changes only. The second rendered retest and final new-source packaged rerun remain pending at this point in the development record.


The second local rendered controlled-aim retest completed at **2026-09-05 10:20:43 UTC with 35 PASS assertions, zero FAIL assertions, and one completion marker reporting zero failures**. Early approach remained 88.565 cm of displacement and 131.435 cm from the initially 220 cm distant target; later pursuit distance was 58.504 cm against the 270 cm limit. Both observations explicitly reported `alive=1 right=0 left=1`. The fixture was subsequently committed as source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`; only `ONEValidation.cpp` changed from S1. This verifies the bounded local rendered fixture retest. The fresh S2 build and packaged suite are still separate pending gates and do not alter the historical S1 failure above.


The later independent **fresh S2 packaged suite passed all 13 modes** at source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`. [packaged_checks.json](packaged_checks.json) records 714 actual PASS assertion lines, zero FAIL lines, one successful own completion marker and exit zero per mode, and all six runtime hashes matching the exact fresh source build. `ONEValidate` passed all 35 assertions. The logs retain 208 categorized warnings (192 socket lookups without a mesh and 16 missing-Recast crowd-manager messages), with no Error, Fatal, assertion-failure or ensure records. This resolves the pending final-rerun gate described earlier; it does **not** rewrite S1's nine-pass/one-fail/three-unrun history. Final performance, visual review and direct manual controls remain separate evidence.
