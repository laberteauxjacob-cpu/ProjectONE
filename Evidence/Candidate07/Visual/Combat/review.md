# Final packaged combat: bounded original-frame review

The six weapon variants have readable held silhouettes, HUDs and shot feedback in the selected final packaged frames. Blood, initial death responses and the shotgun leg/head separations are visible. No obvious material failure, explosive stretch or detached garment appears in this sample.

This review inspected **32 original 1600 × 900 JPEGs chronologically**, including adjacent shot and detached-head frames, from 2,177 captured frames. It is sparse visual sampling, not continuous playback. No audio audition, native-input or performance claim is made.

Source is `3d75c2faa0cecef6075f00755d8e4f5dd2562817`. The bound packaged runner passed **2,119 checks with zero failures**, with clean and unchanged source plus six verified runtime identities. Its ray ledger has 353 rows across all 78 weapon/trial pairs. Exact result, build, runtime, source, timeline and frame hashes are in [review.json](review.json); the complete 32-frame identity/time/observation table is [frame_identities.csv](frame_identities.csv).

| Weapon | Inspected original frames | Main visible result |
| --- | --- | --- |
| M1911 | 223, 224, 225, 231, 324 | Pale tracer, localized blood, ammo and gold hit feedback; head-hit marker partly obscures the buckling target. Frame 231 is a fixture reset. |
| Last Word | 516, 517, 518, 522, 563, 622 | Purple tracer; three struck bodies begin buckling. The low hit leaves the target standing; no detached leg is visible there. |
| M4A1 | 811, 813, 814, 818, 915 | Three struck targets buckle with blood. Frame 915 already shows the next setup, not the prior corpse. |
| Overcurrent | 1112, 1114, 1115, 1119, 1218 | Teal tracer and four initial death responses; head-hit marker obscures some detail. |
| Remington 870 | 1495, 1496, 1497, 1516, 1581, 1680 | Bright muzzle/pellet traces, two floor-level bodies; separate leg with red cut and later detached head/neck cap are visible. |
| Gravebreaker | 1963, 1964, 1981, 2137, 2138 | Orange pellet traces, three floor-level bodies, then a displaced head and red neck cap across adjacent originals. |

The character clothing stays identifiable in these views. Blood and hit feedback remain local enough to read the target silhouette; gold head-hit markers do obscure fine head detail in some pistol/rifle samples. Exact contact, deformation quality between samples and longer settling cannot be established here.

This is an **explicit collision fixture**, not ordinary combat footage: ordinary waves are suppressed, targets stand in declared positions with movement and animation paused, and the player uses scripted projected-cursor/controller input. The inspected penetration trial deliberately gives five lined-up targets **1 health**; the wounded-head trial uses **25 health**. The leg trial uses a 112-health standing target. These images do not establish ordinary-health multi-kill balance or human aim feel.

Each trial clears its prior scene, refills ammunition and resets spread. Frames 231 and 915 visibly contain the next setup while still carrying the previous capture label; this is fixture turnover and must not be presented as corpses vanishing during ordinary play. The short static review also makes no sustained-cadence, recoil-smoothness, audio or long-term corpse-persistence claim. The captured master WAV lasts 79.317333 seconds and was not auditioned.
