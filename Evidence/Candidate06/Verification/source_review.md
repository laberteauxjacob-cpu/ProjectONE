# Candidate06 S3 source review

Source: `c2c422012f4f219fb5e63ac24294a6ba8428f999`  
Compared with: `0890bb7d3b29d4718abc48b7c59b8299e64af49b`

**Static review complete; no actionable finding.** The exact diff contains three files, 12 insertions and one deletion: pickup part placement/rotation, its source-art README and design metadata. Source blob identities and the exact diff hash are retained in `source_review.json`.

Input mappings, controller, player, aim, weapon code and pickup mechanics are unchanged. The inspected source still binds LMB press/release to firing. RMB selects the 168 cm head plane; LeftCtrl selects the 65 cm low plane and wins when both modifiers are held; the default torso plane is 125 cm. Neither modifier has a Fire mapping. Pointer-UI consumption, release suppression, key flushing and weapon eligibility guards remain intact. This establishes no control change from S3; it is not a new native-input test.

The ordinary camera has absolute pitch/yaw/roll `(-58, -90, 0)`: world +X projects right, and world -Y projects up. Pickups spawn with zero rotation, both authored maps use the normal player/game mode, and shake changes camera position only. The new part transform `(X, -Y, Z)` with local yaw `-Yaw` therefore corrects the authored +Y-up layout without reversing left/right order or using negative mesh scales.

The numeral's upper-right segment becomes `(18, -7)` and lower-left becomes `(2, +7)`; its top is at Y=-13 and base at Y=+14. The skull cranium becomes Y=-4 and its jaw Y=+11. The multiplication mark stays on the left. Z, thickness, hover, bounded sway, material parameters and gameplay collision are unchanged. This geometric inference supports the correction on both default-camera maps; rendered legibility still requires actual S3 images.

No engine, build, executable, screenshots, audio or native-input calls were used for this review. No runtime pass or visual-acceptance claim follows from it. Arbitrary future camera rotations or rotated pickup actors are outside this scope.
