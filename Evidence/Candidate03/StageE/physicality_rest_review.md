**Final packaged phase-5 rest review — two bounded intervals.**

Source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`, capture `FinalPackagedPhysicality_e81e1137ed89`. This supplements the [wider physicality review](physicality_motion_review.md) with every saved frame in the two requested windows around the corpse-hit/resume period and late settling.

| Window | Frame IDs | Count | Audio seconds | Phase-relative seconds |
|---|---|---:|---:|---:|
| resume | 01492–01517 | 26 | 55.766130–56.740271 | 7.705746–8.678699 |
| late rest | 01590–01623 | 34 | 59.578320–60.858659 | 11.516136–12.798202 |

All 60 frames were inspected chronologically on six private sheets using crops at native pixel scale. Full original 1600×900 frames `01492`, `01500`, `01517`, `01590` and `01623` were also inspected; they are included in the same 60, not additional coverage. Exact per-frame hashes, times and crop coordinates are in the [manifest](physicality_rest_review.json). Phase-relative time begins at the first captured phase-5 frame, not the exact hit/freeze event.

**Resume interval.** Corpse silhouettes retain their positions and folded limb arrangement. A brief small red particle trace appears near the lower-left corpse after roughly phase+8.17 seconds. No obvious large pose reset, whole-body teleport or explosive pile displacement is visible. The reaction itself is small, so the pictures establish continuity only; they cannot establish a body’s simulation state.

**Late interval.** The visible pile looks quiet through the 1.282066-second window. Bent legs, torso silhouettes and relative positions remain broadly stable, without visible recurring large tremor, body drift or sudden collapse. Some contacts are hidden. Small motion below image resolution and continuing motion elsewhere are not excluded.

The corpses retain different directions and limb arrangements, with localized red patches below them. The upper group is still awkwardly propped and interleaved; crossing limbs and hidden contacts make its weight difficult to read. Visual stability is not final approval of anatomical naturalism or collision-perfect settling.

**Separate numerical evidence.** The actual capture records 249 PASS assertions and zero failures. It reports a fresh hit resuming a naturally qualifying frozen corpse, transition error `0.000000 cm / 0.038784°`, and phase-end `32` awake rigid bodies versus peak `96`, with `3` frozen corpses. These counters do not mean all bodies slept. Supported frozen poses retain physics-only colliders rather than continued simulation. No `corpse_motion.csv` was supplied, and this review does not report or infer individual-body velocities.

Earlier collapse/first freeze, the gap between windows, and other phases are outside this supplemental inspection. No real-time playback, manual control test, audio audition, encoding or Unreal execution was used for this review. Raw grids remain local; only this bounded report and its exact-frame manifest are published.
