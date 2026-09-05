# Original character pipeline

All geometry, skin weights, vertex paint and animation in this folder were authored for Project ONE by `Scripts/create_characters.py`, using Blender 5.1.2. No Project Zero assets, external character meshes, rigs, animation or textures were used.

Run from the project root:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.1\blender.exe' --background --python Scripts/create_characters.py
```

Editable source is in `ArtSource/Characters/Response.blend`, `Infected.blend` and the combined `CharacterWorkshop.blend`. Actions are retained with fake users. `manifest.json` records bones, dimensions, mesh counts and clip durations; `materials.json` records exact native material parameters. FBX exports are in `ArtSource/Exports`.

The source uses centimetres, Blender metric unit scale .01, +X forward, +Y designated right and +Z up. FBX export uses -Y forward/Z up, unit conversion, unit object transforms, no leaf bones, and baked 30fps animation. UE import uses `force_front_x_axis=false`: in-engine transform inspection confirms +X preserved and Y reflected by the coordinate conversion. The native runtime swaps StrafeL/StrafeR accordingly. Anatomical l/r source bone labels therefore appear mirrored in Unreal; the firearm is left-side dominant. The modular sever and remaining-arm animation contract stays internally consistent by bone name. Normalize anatomical labels and authoring axes together in the next pipeline pass rather than renaming only one side of the contract.

Characters use separate compatible naming conventions, not interchangeable bind poses. Import modular infected pieces onto the infected skeleton. Imported vertex colors must replace existing colors; native materials multiply palette base color by vertex color RGB.

The player mesh has 35,524 triangles. Infected body/head/left arm have 16,968/8,896/4,532 triangles. The infected body excludes the head and distal left arm. Both separation boundaries contain closed gore caps and bone sections. Runtime should freeze the evaluated modular skeletal pose for detachment. The optional `SM_Infected_Head` and `SM_Infected_ArmL` fallback exports contain geometry transformed into `head` and `upperarm_l` rest bone local space, with the exact rest matrices recorded in the manifest; a static arm cannot reproduce a currently bent elbow.

`weapon_r` is parented to `hand_r`, at (24,5,131) in the source reference pose. Its local Y points forward, local X points left, local Z points up. A +X-authored weapon therefore needs a local +90 degree yaw before import-axis verification. Support wrist is (42,5,137), corresponding to grip +(18,0,6). Head bone local Y points up; collision offsets along the skull use local Y.

Player clips: Idle 3s, Walk/Back/StrafeL/StrafeR .6s, Run .5s, Fire .2s and Reload 2.1s. Infected clips: Idle 3s, Walk 1.4s, Run .8s, Attack/AttackOneArm 1s, Hit .4s and Death 1.2s. Locomotion targets are 180/370cm/s for player and 100/195cm/s for infected. Attack contact is .48s. All animations are authored in place, with two-bone skeletal leg solves and baked keys; runtime layers upper-body actions over locomotion. Death floor contact is baked from evaluated skin bounds.

`ArtSource/Characters/validate_characters.py` performs read-only source and FBX roundtrip checks. `validation.json` records measurements: all vertices weighted, weight sums within float tolerance of 1, unit rig scale, 180.6cm player mesh height, and sampled locomotion ankle endpoint gaps below .00003cm. Blender source pose PNGs are clearly labelled and are **not in-engine evidence**. Runtime visual validation belongs to the project's gameplay captures.

This is a visually simplified original candidate for review. The infected face, cloth surfaces and hands still need a dedicated sculpt/texture pass to reach the requested grounded production quality.

Actual runtime inspection of `benchmark_6.png` revealed that a full dark crown of hair read as a severed neck disk from the elevated camera. The targeted head revision removes the crown shell, exposes a pale scarred scalp, retains side stubble and broadens the cranium 7%. `-- --head-only` regenerates source files and exports only the infected head meshes, preserving all animation exports for targeted integration.

The subsequent bounded garment revision adds asymmetric sculpted fabric creases, embeds blood into vertex paint instead of raised patches, replaces spherical elbow pads with faceted protection and distributes zipper weights across the spine. `-- --body-only` exports only the player, infected body and arm meshes. These changes preserve the original skeleton and animation contract.


## Candidate05 motion and attack extension

`ArtSource/Characters/C05/README.md` and its inventory document 25 new 100 Hz skeletal clips on the accepted rigs:18 player directional/turn clips, two infected gaits and five left/right/two-hand attack clips across three families. They preserve accepted character surfaces and bind matrices. Source checks report less sustained pelvis compression and exact authored 225/370 cm/s player and 100/195 cm/s infected speeds. C05 uses new asset paths; earlier clips remain historical sources.

The native graph applies short directional live hit reactions additively, preserving movement and attack state for ordinary hits. The infected controller commits attack heading and required limbs, consumes one contact event, checks static cover and caps early step travel. Heavy stagger retains a separate 1.1 s cooldown. New source/native tests and later runtime/visual gates are distinct; generation success is not visual approval. Player damage reaction uses the accepted damage direction and age without taking movement control.
