# Candidate02 captured-frame visual review

Reviewed 13 actual 1600 × 900 packaged-game JPEGs using image inspection from
gameplay build `0637288d32b6fbebc67ef93d4f03e439ff38bb67`. Source:
`Packaged/Candidate02/Windows/ProjectONE/Saved/Candidate02/Comparison`.
Times below are the recorded `audio_seconds` values in that capture's `frames.csv`.
The final capture contains 470 actual frames at 22.38638 frames/second and
20.97066667 seconds of engine audio. These observations concern the captured game
camera, not Blender source previews.

| Sequence | Actual timestamps inspected (seconds) | JPEG frame numbers |
|---|---|---|
| Carbine magazine reload | 5.040183, 5.485417, 5.795558, 6.197262, 6.818888 | 00113, 00123, 00130, 00139, 00153 |
| Shotgun pump | 9.271620, 9.406204, 9.673881 | 00208, 00211, 00217 |
| Shotgun shell reload | 13.436957, 13.704444, 13.972482, 14.778164, 16.754479 | 00301, 00307, 00313, 00331, 00375 |

The carbine remains held in the firing hand while the support arm moves from the
receiver down toward the torso at 5.795558 and returns toward the receiver in the
later samples. The HUD shows 06/192 before the insertion and 24/174 at 6.818888.
The dark magazine is partly obscured by the hand and torso; the handoff is less
readable than the larger arm gesture at this camera distance.

The shotgun has a distinct long, narrow barrel/tube silhouette and brown fore-end.
At 9.406204 the fore-end and support hand sit closer to the receiver than at
9.271620 and 9.673881. Their positions correspond without an obvious large hand
gap in these samples. The knees and feet change pose across the same three
frames while the weapon remains supported. This supports simultaneous locomotion
pose changes during pumping; it does not establish continuous-motion smoothness
or rule out foot sliding between samples.

During shell reload, the gun stays supported and slightly lowered. The support
hand moves toward the hip/torso at 13.704444, then back beneath the receiver at
13.972482, 14.778164 and 16.754479. A small red shell-colored detail is visible
beside the lowered hand, but is only a few pixels across and becomes obscured
near the loading port. Exact finger contact and shell seating cannot be judged
reliably at this scale. The sampled HUD counts rise from 01/036 to 02/035 and
05/032, consistent with incremental loading rather than an instant full refill.
The published `shotgun_reload.jpg` is the inspected 14.778164-second frame, with
the hand beneath the receiver and two loaded shells shown on the HUD.

No gross limb collapse, detached weapon, or large grip separation was visible in
the inspected frames. The main presentation limitation is readability: dark
gloves, magazine and receiver merge together, while shells and mechanical details
are small. The captured character remains visually simplified at this distance.
The sampled reload poses are stationary; these frames do not validate simultaneous
movement during either reload. Full motion playback, other aim directions and
closer inspection would be needed to judge subtle intersections and transitions.

This was a chronological still-frame review, not perceptual video or audio
audition. No audio-quality or audio/animation synchrony claim follows from it.
CSV operation transitions and rendered HUD samples also do not establish exact
subframe timing. No source assets or published frame selections were changed
for this review.
