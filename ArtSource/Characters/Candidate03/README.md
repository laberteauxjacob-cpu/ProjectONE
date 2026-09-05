# Candidate03 locomotion source

`ResponseLocomotion.blend` retains the accepted Response mesh, 21 source bones,
bind matrices, materials and archived editable actions. The established importer
adds the same 22nd armature bone. Only the 18 new action FBXs in
`ArtSource/Exports/Candidate03` are imported by
`Scripts/import_candidate03_locomotion.py`; accepted meshes/actions are not replaced.
Author with `Scripts/create_candidate03_locomotion.py` in Blender 5.1.2.

- Eight direct walking directions: 225 cm/s, 0.72 seconds/cycle, 162 cm/cycle.
- Eight direct running directions: 370 cm/s, 0.62 seconds/cycle, 229.4 cm/cycle.
- Left/right 90-degree two-step turns: 0.60 seconds, with baked counter-rotated
  lower-body poses matching the runtime body-yaw curve.
- Names F/FR/R/BR/B/BL/L/FL refer to **Unreal** direction. Source Y is deliberately
  reflected; there is no second runtime left/right swap for these new actions.

Runtime uses a common distance phase, interpolates adjacent 45-degree clips and
walk/run poses, then counter-rotates the lower body against responsive actor aim.
The retained mesh-space upper-body weapon layers preserve aiming and actions.
Turns start beyond 55 degrees of stationary aim offset; playback accelerates for
faster aim sweeps, and waist offset is capped at 70 degrees. That cap advances the
body reference on instantaneous large aim changes, so fast-pivot contact requires
specific in-engine review. Numeric source checks are not proof of runtime planting.

`inventory.json` records source durations, stride, coordinate convention and the
turn curve. `source_samples.json` and `source_validation.json` check connected leg
chains, loop endpoints, support-foot drift during gait stance, and turn support
positions after the authored body rotation. They do not establish perceived
smoothness, transitions, collision, or gameplay-camera quality.

No external animation, character source or Project Zero content is used. New
binary metadata must pass the repository sanitation helper before publication.
