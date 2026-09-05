# Candidate03 Stage B movement review

The reviewed directional and turn defects pass this bounded motion gate. Abrupt aim changes now retain the evaluated feet before a staggered recovery step; the earlier one-frame 18 cm foot swap is absent from the corrected probes. This is technical motion verification, not approval of final animation naturalism or packaged testing.

Two actual Unreal game-camera captures supply the evidence. `MovementCapture` contains 3,659 frames at 1600×900 and uses the current 18 imported locomotion clips, before the final mesh tick prerequisite correction. It supports steady directional review only: its abrupt-turn defect was found and superseded. `MovementCapture_Turns` (Turn3) contains 1,589 frames after the correction and supplies the accepted turn evidence. Exact CSV SHA-256 identities, timestamps, per-trial measurements and individual reviewed frame names are in [movement_metrics.json](movement_metrics.json). The first, older-import capture is excluded.

## Evaluated measurements

- All 48 directional trials passed: eight directions × two weapons × walk, sprint and reload-walk. Recorded steady speeds were 225 cm/s for walk/reload-walk and 370 cm/s for sprint, including diagonals. See [directional_checks.txt](directional_checks.txt).
- Turn3 contains twelve abrupt 180° aim changes across both weapons, including the rapid midstep return. Both feet held their previous XY/Z values at the recorded precision on eleven changes. On the remaining change, an already-lifting foot moved 0.101 cm horizontally and 0.575 cm vertically; the other foot held. Subsequent 220 ms windows had maximum per-sample recovery movement of 2.62–5.44 cm, rather than the previous 18 cm swap and return.
- In trimmed continuous CW/CCW 90°/s and CW 180°/s windows, no sample reached the 69.9° aim/body clamp boundary. Maximum offsets were 55.91°/33.55°/38.36° for the carbine and 55.35°/56.23°/40.26° for the shotgun. Abrupt input jumps still use the intended 70° bound with world-foot compensation.
- The low-joint-height support proxy had maximum XY diameters of 2.46 cm walking, 2.42 cm during reload-walk and 7.05 cm sprinting. This proxy includes possible low swing tails and does **not** measure shoe-sole contact or establish zero sliding. Joint lift and firing assertions also passed in [focused_turn_checks.txt](focused_turn_checks.txt).

## Actual chronological image inspection

The character reviewer inspected seven full gameplay images in each of eight carbine sprint directions (56 images), at world-time windows F 14.062–14.716, FR 15.483–16.105, R 16.897–17.544, BR 18.314–18.924, B 19.725–20.363, BL 21.126–21.770, L 22.535–23.176 and FL 23.949–24.575 seconds. These roughly one-cycle windows show alternating extension/recovery while the upper body retains aim.

Another 56 images cover all eight shotgun sprint directions, including the settling-to-stride transition: F 55.881–56.515, FR 57.289–57.940, R 58.679–59.329, BR 60.131–60.787, B 61.555–62.173, BL 62.977–63.592, L 64.393–65.018 and FL 65.827–66.476 seconds. Alternating leg motion and the shotgun hold remain visible, without a new sampled whole-body snap or inverted joint. These shorter moving portions are not claimed as complete steady cycles.

The integration reviewer independently inspected five chronological gameplay-frame crops in every walk and reload-walk direction with both weapons: 160 crops across 32 sequences. Alternating extension/recovery and the respective hold/reload action were visible in all directions, without whole-body snapping in those samples. Exact selections are in [walk_reload_visual_manifest.json](walk_reload_visual_manifest.json).

Turn3 inspection covered carbine upper-aim/step windows 3.983–5.280 and 6.994–7.682 seconds, plus 43 consecutive images around carbine settled/midstep reversals at 13.548–13.776 and 17.271–17.769 seconds and shotgun reversals at 29.795–30.027 and 33.462–33.999 seconds. Gun heading changes immediately; feet retain their positions into staggered leg repositioning. The integration reviewer independently inspected every frame 00294–00308 (13.508519–14.044655 seconds), reaching the same conclusion. Representative unaltered frames: [before](turn_before.jpg), [heading changed](turn_heading_changed.jpg), [recovery](turn_step_recovery.jpg).

## Remaining limitations

Low hips, deep knee compression, long strides and intermittent leg overlap still produce a visibly crouched, mechanical gait; [this actual shotgun frame](shotgun_stride_compression.jpg) preserves that shortcoming rather than hiding it. The shotgun shoulder silhouette is also stiff in some turn views. The camera distance and occlusion limit fine sole and hand-contact judgments. Chronological still samples are not continuous playback, and this review does not certify every transition, arbitrary aim pattern, audio quality or final packaged behavior.
