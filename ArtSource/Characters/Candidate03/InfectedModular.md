# Candidate03 modular infected source

`InfectedModular.blend` derives the accepted Project ONE `Infected.blend` without
regenerating the character. It retains the original 21 source bones, bind pose,
materials, vertex paint and seven infected actions. FBX adds the same existing
armature node in Unreal. The accepted head is unchanged. Hidden reference meshes
remain editable in the source; export selection includes only the intended new
mesh and shared rig.

| Mesh | Unreal anatomy | Bone chain |
| --- | --- | --- |
| SK_Infected_Core | torso, retained stumps and intact right leg | shared full skeleton |
| SK_Infected_Head | head | head |
| SK_Infected_ArmLeft | left arm | upperarm_r / lowerarm_r / hand_r |
| SK_Infected_ArmRight | right arm | upperarm_l / lowerarm_l / hand_l |
| SK_Infected_LegLeft | distal left thigh, knee, shin and foot | thigh_r / calf_r / foot_r / toe_r |

Meshes import under `/Game/ONE/Characters/Candidate03/`, reusing
`/Game/ONE/Characters/SK_Infected_Skeleton` and the existing material assets.
The established conversion keeps source +X forward and reflects Y. Thus source
`_r` is anatomical left in Unreal; no bone renaming is performed.

Both shoulder cuts are 4.8 cm down their upper-arm bone. The left thigh cut is
38% down thigh_r, at UE component coordinates (0.456, -9, 79.04) cm. The new cut
geometry has shared rim positions/weights, recessed tissue rings, varied vertex
paint and a small bone section. The intersecting cargo pocket is split and
closed too. The old right-arm cap and short proximal shoulder stump are replaced
to share an identical rim. No exterior surfaces outside the defined cut regions
are changed. The accepted closed head/neck caps are retained.

`infected_inventory.json` contains exact cut points, normals, source-bone-local
offsets, rest matrices, radii, file paths and material slots. It is the attachment
and physics-authoring contract. Keep cut-root bones evaluated and their original
stump weights; hiding or scaling these bones would erase the stump. Physics-chain
removal is a separate runtime action. The core remains the full-pose leader;
visible modular followers do not independently simulate while attached.

`A_Infected_C03_AttackRight` imports under `/Game/ONE/Animations/Candidate03/`.
It strikes with source hand_l, the actual right arm. It is authored at 100 Hz,
lasts 1.0 second and contacts at 0.48 second. Its evaluated source-left/right bone
deformation is mirrored through the unchanged bind matrices, preserving proper
bone orientation. At contact the active hand is approximately 59.0 cm forward
and the other hand approximately -4.6 cm forward. The retained
`A_Infected_AttackOneArm` supplies the anatomical-left strike. Select the action
from actual presence; both-arm, head and selected-leg loss are fatal under the
Stage D runtime contract. Original editable actions retain their 30 Hz timeline;
the new action records its own authored FPS/contact metadata.

Generate and validate with Blender 5.1.2:

```text
blender --background --python-exit-code 1 --python Scripts/create_candidate03_infected.py
blender --background --python-exit-code 1 --python Scripts/validate_candidate03_infected.py -- --render
```

The generator writes five mesh FBXs and one animation FBX under
`ArtSource/Exports/Candidate03/`. Root coordinates
`Scripts/import_candidate03_infected.py` in the Unreal commandlet and separately
creates/tunes physics assets; the importer never creates a default physics asset.
Run the repository metadata sanitizer after generation/rendering and before
import hashes/publication are finalized. Regenerating later requires another
sanitation pass and appropriate reimport.

Source validation: all 168 sampled limb-seam evaluations across eight retained/new
actions have zero measured rim gap; weights normalize within float tolerance.
Exactly 14,369 of 14,598 accepted polygons retain positions, weights, material and
corner colors. The other 229 are confined to permitted cuts/caps; there are zero
unauthorized changed polygons. The head face data is identical. FBX roundtrip
checks preserve all bone names, paint layers and dimensions, with errors below
0.005 cm; the complementary action roundtrips at 100 Hz and 1.0 second.
`infected_validation.json` and `infected_roundtrip.json` contain the measurements.

The three `Blender_InfectedModular_*.png` files are source inspection renders,
including an exploded cap view and the right-arm contact pose. They are not
gameplay evidence. The accepted character remains visually simplified; these
changes establish modular geometry and an animation contract, not a broad art
redesign. Unreal pose transfer, physics fitting, side-specific hit registration,
severing, contact and cleanup still require their separate runtime checks.

All source geometry, animation and paint remain original Project ONE work.
No external character, animation, texture pack or Project Zero content is used.
