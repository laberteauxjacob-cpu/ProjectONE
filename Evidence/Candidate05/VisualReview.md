# Candidate05 development visual review

This review covers actual offscreen editor-game rendering from the evolving Candidate05 working tree, descended from the preserved Candidate04 baseline (`candidate04`, `2a75a4a5c09f52dacf289ced1b91d548ce3f2a3d`). It does **not** bind a packaged executable or certify native input, continuous playback, audio quality, or final visual approval.

The successful motion capture is `Saved/Candidate05/MotionCapture/20260905_213347_2085B276`: 1,208 genuine JPEGs and **80/80 scripted assertions passed**. Visual inspection covered **496 distinct consecutive captured frames**: 241 turn/attack/hit frames and 255 stride frames, with 17 of those also viewed as full 1600×900 images. Crops retain actual gameplay-camera pixels; labels were added outside the pixels. Raw captures and private logs are not published here.

All indices below mean `frame_NNNNN.jpg` in that capture. Exact audio/world/phase timestamps, crop rectangles and source SHA256 hashes remain in the portable private records `Saved/Candidate05/Review/MotionCapture_2085B276/review.json` and its `Strides/review.json` supplement.

| Direction | Walk, phases 0–7 | Sprint, phases 8–15 |
|---|---|---|
| Forward | 00009–00025 | 00318–00332 |
| Forward right | 00048–00064 | 00357–00371 |
| Right | 00086–00102 | 00395–00409 |
| Back right | 00125–00141 | 00434–00448 |
| Back | 00163–00179 | 00473–00486 |
| Back left | 00202–00218 | 00511–00525 |
| Left | 00241–00257 | 00550–00564 |
| Forward left | 00280–00296 | 00588–00602 |

Each steady window exceeds one authored stride cycle. All directions show alternating extension and recovery with stable pistol/body heading. Supporting legs straighten during portions of the cycle rather than remaining in a constant low crouch. Deep recovery bends, lateral/diagonal leg overlap and wide backward-running steps still look mechanical. No wrong-way body rotation or full-body reset was observed inside these windows. Small foot slide remains possible; there was no matched earlier runtime A/B inspection to quantify improvement. This coverage uses the starter pistol ready pose, not separate rifle, shotgun or reload strides.

Turns cover 00624–00642 and 00684–00704; rapid pivots cover 00768–00778, 00785–00794 and 00802–00811. Both feet visibly clear during staggered release without an apparent planted-foot teleport. Recorded rapid-pivot lift ranges are 5.898/7.320 cm; sampled full-support positions hold at CSV precision. These observations do not establish obstacle-aware foot planting.

Swipe 00906–00937, rake 00966–00999 and two-hand 01026–01061 show anticipation, weight transfer, reach and recovery. Each scripted family dispatches one 19-point contact. Swipe/rake silhouettes remain similar at this camera scale, joints and squat stances remain angular, and overlapping bodies obscure precise palm contact. The complementary arm variants and natural encounter selection were not independently reviewed here.

Frames 01091–01116, 01124–01144 and 01148–01168 cover minor reactions, kill/collapse and the later corpse shot. The infected continues approaching through minor reactions; telemetry records four LiveHit outcomes, one NewKill and one CorpseHit. The visible kill marker and collapse agree with those events. The player retreats to approximately 261 cm before the corpse shot, so this does not demonstrate point-blank low-corpse accuracy or high-rate rifle resistance to repeated hit reactions.

Environment's independent `Saved/Candidate05/Review/UI1600x900_Round0/review.json` records six full-resolution UI states (`Saved/Candidate05/UI1600x900/01_starter_0_7_56.png` through `06_restarted_starter.png`) and 49/49 UI assertions. Starter, expanded tools, points, pause, death and restart text were readable without observed clipping; round zero was consistent. Its seventh still, earlier capture `20260905_212158_E62246F9/frame_00983.jpg`, did not reproduce the initial missing-glyph impression. This is bounded still-image evidence, not approval of every resolution or the death transition.

Earlier attempts remain failed evidence: capture `20260905_212158_E62246F9` had 75/80 passing assertions, insufficient trailing-foot lift and a failed kill/corpse chain. Those findings led to positional-target IK, production aim corrections and a feasible retreat fixture before the successful capture above. The later result does not erase the failed run or establish equivalence with future packaged builds.
