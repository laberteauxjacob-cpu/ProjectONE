# Candidate03 final native capture: bounded motion review

Source: `e81e1137ed891570c73c8c278fc2c8cc2250bd04`. Capture: `Manual/20260905_110414`, the final packaged game observing ordinary native input. The recorder did not steer the player.

Reviewed **245 distinct consecutive gameplay frames across six windows**, using 18 private chronological contact sheets. Nine of those frames were also inspected at their original 1600×900 resolution. This is a bounded image review, not a real-time playback or perceptual audio audition. The complete capture contains 4,970 frames.

| Window | Inclusive source frames | Audio seconds | Count |
|---|---|---:|---:|
| carbine turn fire 1 | `frame_00131.jpg`–`frame_00165.jpg` | 4.709532–5.966255 | 35 |
| carbine turn fire 2 | `frame_00313.jpg`–`frame_00352.jpg` | 11.354034–12.788739 | 40 |
| shotgun bright | `frame_00843.jpg`–`frame_00872.jpg` | 30.654977–31.716909 | 30 |
| shotgun dim | `frame_02297.jpg`–`frame_02326.jpg` | 83.423382–84.484662 | 30 |
| infected approach | `frame_03228.jpg`–`frame_03287.jpg` | 117.017599–119.171440 | 60 |
| player death | `frame_03439.jpg`–`frame_03488.jpg` | 124.715971–126.499011 | 50 |

Times come directly from `frames.csv`; all rows have phase `-1`. The labels above identify review windows, not scripted phases. The [JSON manifest](native_motion_review.json) records crop bounds, exact endpoints and sparse representative source-frame hashes. Native-resolution frames inspected: `frame_00137.jpg`, `frame_00319.jpg`, `frame_00320.jpg`, `frame_00847.jpg`, `frame_00860.jpg`, `frame_02301.jpg`, `frame_03282.jpg`, `frame_03460.jpg`, `frame_03480.jpg`.

## Visible findings

The two brief carbine aim changes show prompt gun-heading changes followed by small foot adjustments. The second reverses heading between frames 00319 and 00320. No large full-body teleport is apparent in the sampled images, but the upper-body change is abrupt and these images do not measure world-space foot planting. In frame 00137, the small pale muzzle flash appears attached to the current muzzle. A separate thin world-space tracer is visible above it on the fired trajectory; it is intended to stay on that trajectory rather than follow later gun rotation. Their visible separation alone is not evidence of a flash-attachment regression. The precise temporal cause of the trajectory/current-heading difference was not established by this image review.

Both shotgun shots have a short visible flash. The dim shot at frame 02301 briefly lights the nearby floor/arm; the flash does not persist through subsequent captured frames. The bright pump sequence includes a tiny red hull visible at native scale in frame 00860. Support-hand and fore-end movement is subtle at this camera distance, especially in the dim sequence. Precise grip contact and the eventual casing landing remain unverified.

The infected advances across open floor with alternating strides, reaches the stationary player, then begins attacking. The low, long stride remains mechanical, and the shotgun silhouette overlaps the infected at close range. During the later attack, the player remains upright until the RESPONSE LOST overlay first appears in frame 03465 at audio 125.667172 seconds. The overlay then hides the characters; this review does not establish a player-collapse animation.

## Scope and limits

The selected windows contain four actual shots, consistent with the separate [native input review](native_input_review.md). Later clicks after death are not additional shots. Reload windows were not part of this dense review, and no reload-hand-quality claim follows from their occurrence in the full recording. Held W+Shift, sustained turning, every-direction motion, fine finger contact, collision correctness and exact input latency are outside this image review.

The observer recorder’s “Failures 0” status is not another automated assertion suite. The [packaged checks](packaged_checks.json), [native audio measurements](native_audio.json) and [movie assembly record](native_capture_assembly.json) report their separate scopes. No perceptual audio-quality approval is claimed.

Raw frame-grid files remain local. `frames.csv` SHA-256: `4c786f76e965a849b58f8c7b7fd735bf21026b2287a491e4ed6e55b0298d6864`. Review status: complete for the stated coverage, with the concrete presentation limitations above retained.
