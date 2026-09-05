# Candidate03 baseline — Candidate02 player corrections

Baseline source/tag: `aa4d55d04bf375bbef41362af77eec10d9ea224f` / `candidate02`.
The existing Candidate02 package is preserved; its main executable SHA-256 is
`53966e40e59f9a0ca99ddabc6beec0199ee8ffda807abeb9d9187709617c0350`.
Candidate03 starts on `codex/candidate03`. No prior Candidate03 work was present.

The supplied attachment folder contained the consolidated instructions only.
The four photographs and movement video described there were not available.
The written playtest findings are treated as requirements, irrespective of older
functional PASS reports.

## Direct baseline observation

The user's existing Candidate02 game was already open and paused in the sandbox.
After resuming, seven dead infected displayed repeated, nearly parallel poses.
[This unchanged gameplay-camera capture](baseline_repeated_corpses.jpg) records
the actual state. F5 reset the encounter. Keyboard 2 selected the shotgun; Tab
returned to the rifle; mouse wheel selected the shotgun; keyboard 1 returned to
the rifle. The mesh, name, selected slot and ammunition display changed together.
The compact original slot text did not make shotgun access prominent.

## Confirmed source causes

- Player WalkSpeed is 180 cm/s; infected maximum pursuit is 195. Reload explicitly
  forces WalkSpeed even while Shift is held. No automatic reload path exists.
- Side/back sprint uses the walking clips. The current 108 cm walking stride at
  370 cm/s produces about 3.43 cycles/s, versus 2.00 for forward running. The
  diagonal DirectionScale can raise this further. These are animation-rate
  findings; physical diagonal movement must be measured separately.
- Stationary aim interpolates actor yaw without a stepping or foot-yaw solution.
- The existing gun-attached point light is separate from a world-space shot
  effect. That effect creates a constant yellow ray plus a wide 19 cm origin
  segment for 0.045 seconds; it cannot follow a moving muzzle.
- Shot audio already has three candidates and small pitch variation. Candidate
  files share their envelope/layer/tail design with different noise seeds;
  selection permits immediate repetition. No perceptual approval is inferred.
- The shotgun already ejects once at 0.18 seconds into its pump. Rifle brass is
  absent. Existing cases have fixed motion, no inherited velocity and stop on
  the first static contact rather than bouncing.
- Death selects one authored death animation; there is no active ragdoll physics
  asset. Only head and one arm have separable visual modules/hit regions.
- Corpse blood is a fixed-size spawned decal. There is no finite wound leakage,
  trail or growing corpse-pool state.
- RVO uses the 27 cm capsule with the engine's default 1.5 avoidance expansion,
  producing larger virtual spacing than physical contact. Actual contact and
  pursuit behavior still require combined runtime testing after changes.

This record establishes the starting point. Candidate03 results are maintained
in [the pass report](../../Docs/Passes/Candidate03.md); none of the proposed
corrections above is considered verified merely because its code is present.
