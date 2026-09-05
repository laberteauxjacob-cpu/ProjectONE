# Stage D intermediate rendered-motion review

**These captures do not close Stage D.** Capture 3 substantially improves visible floor pools, and the reviewed falls produce different corpse arrangements. Settling still fails, and some detached-leg contacts remain obscured. This review is technical observation, not user approval of the mechanics or visual direction.

Captures 1–3 were actual editor-game recordings of an evolving working tree. Capture 3 was locally labelled build 13; it is **not tied to an exact source commit or a fresh packaged build**. Later accepted editor and fresh-package evidence must be recorded separately. No result from those later runs is implied here.

| Capture | Saved frames | Automated checks / failures | Dense frames actually inspected | Additional sparse context views |
|---|---:|---:|---:|---:|
| PhysicalityCaptureRun1 | 3,067 | 238 / 1 | 535 | 45 |
| PhysicalityCaptureRun2 | 3,096 | 235 / 1 | 212 | 27 |
| PhysicalityCaptureRun3 | 3,121 | 239 / 1 | 136 | 18 |

All three failures were the six-death sequence ending with 96/96 bodies awake. The dense review used every consecutive saved frame in each listed window, viewed through chronological crops/contact sheets. Sparse context views overlap some dense windows and are not added as distinct frames. Native-size frames supplemented those views. No scene synthesis, interpolation, sharpening, motion reconstruction, perceptual audio audition, or direct manual playtest was used for this review.

The [machine-readable manifest](visual_review_manifest.json) records capture-file hashes, exact source filenames, world/audio clocks, selected-image hashes and review counts. Raw captures remain preserved separately under their respective `Saved/Candidate03/PhysicalityCaptureRun1`, `Run2`, and `Run3` directories. The four public JPEGs below preserve the original 1600×900 image bytes.

| Image | Capture 3 source file | Audio seconds | What the image supports |
|---|---|---:|---|
| [Pool early](capture03_pool_early.jpg) | frame_00261.jpg | 10.541021 | Initially small exposed pool edge beside the torso. |
| [Pool late](capture03_pool_late.jpg) | frame_00472.jpg | 18.934040 | Later clearly readable dark-red footprint beneath/around the torso. |
| [Different corpse arrangements](capture03_differing_corpses.jpg) | frame_01551.jpg | 60.619443 | Different physical arrangements and visible floor pools; not a settling pass. |
| [Six-subject crowd setup](capture03_crowd_setup.jpg) | frame_01214.jpg | 48.322667 | Six live subjects in the controlled death fixture; not proof of navigation or avoidance. |

## Findings and unresolved limits

**Capture 1:** The torso and six-death sequences fold from visible moving/standing poses without an obvious bind-pose reset or large launch in the inspected windows. Corpse arrangements differ rather than repeating parallel poses. Both arm cases visibly separate at the corresponding shoulder, remain absent on the survivor, and do not reappear during the later collapse. The survivor approaches and extends the remaining arm. The head becomes a separate visible part. Exact cap closure, small finger contacts, and some joint limits cannot be certified at this scale. The detached leg stays close to/overlapping the corpse, obscuring the knee, foot and proximal stump support. Dense late frames show residual torso/shoulder/hand twitching in the crowded screen-left body. Pool growth is reported numerically but a broad growing footprint is difficult to see.

**Capture 2:** The pool remains mostly concealed by the torso, with small red spots more legible than the main footprint. The leg cut is briefly visible and both subjects fall without an observed large part launch or limb reappearance; overlapping bodies still prevent a reliable detailed leg-contact assessment. Most late corpses look quiet, but a small head/shoulder adjustment remains beside the player's screen-left leg. Separate telemetry in the inspected late interval records one corpse reaching 41.970337 cm/s and a sampled pose step of 4.744812 cm / 21.550217 degrees. These are physics-body measurements, not distances derived from pictures.

**Capture 3:** The revised pool material makes the dark-red torso footprint readable by the middle/late windows. Pools beneath the lower bodies in the six-death scene are also unmistakable. The previously suggested radius-factor increase from 6 to 9 is unnecessary solely to address the former invisibility in these views: this run retains factor 6 and the finite blood budget. Flat color, stamp-like borders and regularly spaced deposits remain stylistic limitations. The body footprint and pose differ between runs, so this is not a controlled pixel-for-pixel material-only comparison.

The Capture 3 late pile looks mostly quiet, with subtle limb adjustments and no obvious large rolling in the inspected interval. Matching telemetry peaks at 6.520179 cm/s, 0.389412 cm sampled pose change and 4.442343 degrees. All six still end with 16 awake bodies. One corpse records a supported-guard sleep request followed by a contact wake; the other five record no manual sleep. That does not establish a successful settling fix. Tightly folded limbs, raised/occluded contacts and the dense upper pile also limit any claim of natural relaxation or penetration-free contact. Phase 4 was not newly inspected in Capture 3; its earlier occlusion limitation remains.

The later supported-pose freezing implementation was **not present in these reviewed captures**. This report neither validates its transition continuity nor accepts its gameplay tradeoff. Re-hit/sever resumption, physical contact behavior, final motion, manual controls and fresh packaged performance require their own evidence. No overall mechanics, settling, detached-leg contact, or user-approval gate is marked passed here.

## Exact chronological inspection windows

Frame ranges are inclusive. Each number denotes the original filename `frame_NNNNN.jpg`. Times are the capture's recorded audio clock, not a claim of exact damage-event timestamps. Phase-relative setup can differ from the first labelled screenshot by one frame.

| Capture | Phase / window | Files: first–last | Audio seconds | Count |
|---|---|---|---|---:|
| 1 | 1 torso wound/collapse | 00187–00264 | 7.622374–10.661745 | 78 |
| 1 | 2 anatomical-left arm sever | 00491–00523 | 19.639930–20.918694 | 33 |
| 1 | 2 survivor / later collapse | 00526–00602 | 21.039358–24.035068 | 77 |
| 1 | 3 anatomical-right arm sever | 00745–00778 | 29.633089–30.922533 | 34 |
| 1 | 3 survivor / later collapse | 00780–00857 | 31.000391–34.046087 | 78 |
| 1 | 4 head and leg loss/falls | 00999–01076 | 39.618061–42.679785 | 78 |
| 1 | 5 six staggered deaths | 01227–01306 | 48.658715–51.695607 | 80 |
| 1 | 5 later settling | 01341–01371 | 53.086686–54.287660 | 31 |
| 1 | 5 late contact | 01490–01535 | 59.088090–60.920540 | 46 |
| 2 | 1 pool early | 00259–00285 | 10.461723–11.489853 | 27 |
| 2 | 1 pool middle | 00350–00376 | 14.057110–15.086900 | 27 |
| 2 | 1 pool late | 00443–00473 | 17.738043–18.921161 | 31 |
| 2 | 4 head and leg loss/falls | 00997–01074 | 39.631052–42.709643 | 78 |
| 2 | 5 late contact | 01495–01543 | 59.076976–60.907248 | 49 |
| 3 | 1 pool early | 00259–00286 | 10.460499–11.533183 | 28 |
| 3 | 1 pool middle | 00350–00376 | 14.075002–15.116493 | 27 |
| 3 | 1 pool late | 00443–00472 | 17.780122–18.934040 | 30 |
| 3 | 5 late contact | 01510–01560 | 59.139746–60.954169 | 51 |

Additional native-size inspection: Capture 1 frames 00463, 01040, 01076, 01527; Capture 2 frames 00473, 01028, 01074, 01196, 01515; Capture 3 frames 00261, 00472, 01551 and, during public curation, 01214 (already seen in the sparse context sheet). These stills support detail/context only; motion findings rely on the consecutive windows above.


## Capture 4: minor-wound trail visibility

This phase-0 review uses the actual editor-game Capture 4 from local working-tree **build 15**, without an exact source-commit or packaged-build claim. All 100 consecutive frames **00030–00129** were inspected as chronological native-resolution crops; recorded audio time is **1.453020–5.330768 seconds**, and world time is **6.437022–10.313860 seconds**. Full 1600×900 views of 00000, 00030, 00080 and 00160 supplement the sequence; two overlap the dense window, giving 102 distinct source frames. The [minor-trail manifest](capture04_minor_review.json) records every inspected source hash and finalized capture clock.

With the infected starting at X360 in the open lane, two separated dark-red floor marks appear behind it during pursuit. The first resolves during frames 00037–00042 and the second during 00058–00064. Both remain visible as the infected closes to melee and are readable in the full native views. This demonstrates a short, sparse minor-wound trail; it does not establish an unbroken trail or prolonged bleeding. No bench or HUD hides the marks in the reviewed main sequence.

The opening frame contains camera recentering and preparation text. A brief faint square near the first deposit resolves as its dark mask appears. The established marks show no persistent floor flicker or texture swimming in the inspected sequence. This section covers minor-trail visibility only and adds no later pooling, freeze/resume, performance, audio-audition, manual-playtest or overall Stage D verdict.
