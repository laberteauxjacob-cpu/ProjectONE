**Final packaged blood, crowd and cleanup review — bounded coverage.**

Source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`; capture `FinalPackagedPhysicality_e81e1137ed89`. The [fresh build record](fresh_build.json) identifies the packaged source. The capture contains 3,069 original 1600×900 JPEGs; its separate `checks.txt` contains 249 PASS assertions and zero failures. Those numerical checks are not treated as visual approval.

I inspected all 333 consecutive saved frames in the 11 windows below, using 22 private chronological crop sheets, plus 11 original native frames (337 distinct source images overall). Crops were only resized for inspection. No generated imagery, interpolated frames, encoding, real-time playback, manual control test or audio audition was used. Exact per-image SHA-256 values, CSV times, crops and source-record hashes are in the [review manifest](physicality_blood_crowd_review.json). Raw grids are not duplicated in the public repository.

| Phase / window | Consecutive frame IDs | Count | Audio seconds | Phase-relative seconds |
|---|---|---:|---:|---:|
| 0 / minor trail | 00055–00136 | 82 | 2.028396–4.991491 | 2.013570–4.976781 |
| 1 / pool early | 00301–00322 | 22 | 11.045665–11.825781 | 4.006301–4.786909 |
| 1 / pool late | 00484–00504 | 21 | 17.768391–18.512240 | 10.728750–11.472593 |
| 6 / corner approach | 01655–01687 | 33 | 62.117141–63.381025 | 1.025863–2.289596 |
| 6 / corner late | 01852–01871 | 20 | 69.921649–70.672464 | 8.828552–9.581153 |
| 7 / rack approach | 01933–01965 | 33 | 73.119311–74.385525 | 1.016807–2.281196 |
| 7 / rack late | 02127–02146 | 20 | 80.911587–81.680520 | 8.806045–9.575256 |
| 8 / bench approach | 02207–02238 | 32 | 84.120409–85.363384 | 1.032929–2.275566 |
| 8 / bench late | 02401–02420 | 20 | 91.898572–92.658503 | 8.809578–9.569596 |
| 10 / cleanup early | 02986–03013 | 28 | 114.132982–115.112344 | 0.000000–0.985603 |
| 10 / cleanup late | 03047–03068 | 22 | 116.341072–117.102929 | 2.213455–2.974625 |

Phase-relative time starts at the first captured frame of the phase, rather than its exact gameplay event. The gaps between windows were not reviewed.

**Minor trail.** Sparse, small dark-red floor stains remain fixed as the wounded infected walks away. A second stain becomes clearer during frames 00069-00075; the earlier stain persists. The attack effect near the player later in this window is not counted as another wound-trail deposit. The selected interval shows a few deposits, not a continuous stream. It does not establish the full bleeding lifetime.

**Pool growth.** Compared with the early window, a wider red patch is exposed behind the shoulder/head in the late window and native frame 00504. The pool remains partly covered by the torso; separate smaller trail stains stay visible to its right. The gap between the two dense windows was not reviewed. These images do not measure growth rate or prove continuity throughout the gap; a large fraction of the pool is occluded.

**Corner crowd.** Visible infected advance toward the corner player with changing leg poses, then form a compact ring/queue. The late sequence shows local stepping and attacks while the player remains cornered. Rear actors and feet overlap. Some queued actors continue stepping with little translation. The camera exposes a large black area outside the room.

**Rack routes.** The early sequence shows actors approaching around the left and right rack edges. Later, a queue occupies the wall-side passage and the aisle behind the racks. No gross through-rack jump is visible in these sampled frames. Racks, other actors and the HUD occlude feet and several complete paths. The images cannot prove that every one of the 18 actors has an unobstructed route or never stalls. Dense queuing and overlapping silhouettes remain visible.

**Bench crowd.** Actors approach from the open floor and gather around the exposed end and sides of the bench. The late window retains visibly different positions around the bench, with a trailing queue and close-contact animation. The close group obscures the player and individual foot contact. Long, low strides and repeated body silhouettes remain mechanical in appearance; this is not a final naturalism approval.

**Cleanup.** Both inspected windows show the player on a cleared floor with no visible corpse, severed part or blood reappearing. The early camera settles after the phase transition; native final frame 03068 shows zero remaining on the HUD. The phase changes the player/camera position. The view alone cannot establish off-camera actor or component cleanup; that is a separate automated assertion.

Native-pixel checks: `00082`, `00268`, `00504`, `01705`, `01860`, `01983`, `02136`, `02256`, `02410`, `03000`, `03068`. Their exact timestamps and hashes are listed separately in the manifest.

The reviewed coverage supports visible localized bleeding, exposed pool growth, crowd approach through the available lanes, and a clear visible floor after cleanup. It retains substantial presentation limits: close-contact overlap, repetitive low strides, partial foot/route occlusion, and off-map black camera space near corners. This review does not certify all-actor pathing or collision, physical settling, frame-time performance, or final visual naturalism. Phases 2–5 and the separate manual review remain the integration lead’s scope.
