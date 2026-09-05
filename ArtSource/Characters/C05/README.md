# Candidate05 character motion

The two editable sources derive from the accepted Candidate03 ResponseLocomotion and InfectedModular files. Mesh vertices, weights, material assignments and all 21 authored bind matrices remain unchanged. Unreal adds the armature node when importing onto the existing skeleton. No earlier source/action asset is overwritten.

`Scripts/create_candidate05_motion.py` creates 25 original animation-only FBXs at 100 Hz. `Scripts/import_candidate05_motion.py` imports them into `/Game/ONE/Animations/Candidate05` against the existing Response/Infected skeletons. Root coordinates the Unreal process. `inventory.json` binds final sanitized FBX bytes; `source_validation.json` records source checks and `source_samples.json` keeps sampled joints.

Player: eight independently authored walk directions at 225 cm/s (.64s cycle), eight run directions at 370 cm/s (.54s), and the existing two-step 90 degree turn schedule (.60s). The shorter strides, 2.2/3.2 cm initial pelvis compression, supporting-leg weight transfer, ankle recovery/roll and small counter-rotation replace the previous constant 9/10 cm lowering. Reach solving can still lower the pelvis at extreme extension. The runtime pivot preserves world feet with a 1.5 cm pelvis offset instead of 5 cm. These are source geometry changes, not a movement-speed increase.

Infected: new 100 cm/s walk (1.04s) and 195 cm/s run (.66s) retain slightly forward torso weight and asymmetric opposing arm travel. Five clips implement three attack families:

| Family | Clips | Duration | Contact | Early step budget |
|---|---|---:|---:|---:|
| Swipe | SwipeLeft / SwipeRight | .96s | .45s |18 cm, ends .34 s |
| Cross-body rake | RakeLeft / RakeRight |1.08s | .48s |12 cm, ends .34 s |
| Two-hand strike | TwoHand |1.12s | .54s |14 cm, ends .38 s |

All names start `A_Infected_C05_`. Source `_r` is anatomical LEFT after the established Y reflection; source `_l` is anatomical RIGHT. Clips include preparation, torso/hip rotation, foot lift, reach, follow-through and recovery. Active wrist positions at contact are recorded in the source validation. The two one-arm families remain available when either arm is lost; a two-hand action requires both throughout its pending contact.

The controller commits heading once, moves through CharacterMovement collision using a bounded non-homing early step, consumes one contact event, and checks required arms, world-static cover, 96 cm center reach, 75 cm height and a forward cone. Lost required limbs consume pending contact; death or a qualifying full stagger cancels attack. Minor accepted live hits apply a short directional pose impulse without changing state or attack cooldown. Only a qualifying heavy packet can enter the .52s full stagger, with 1.1 s cooldown. Damage remains 19; player .55s protection is unchanged.

Audio hooks notify the separate C05 zombie component once at windup, once per accepted live transaction (component throttled), and once on live-to-dead transition. Corpse packets do not replay living reactions or kill feedback. The four new native automation cases cover contact timing, single-hit behavior, wall/dodge/limb/death cancellation, outcome separation, minor-state continuity and the finite step integral. These are authored tests, not claims that execution has passed.

Source numeric checks pass for finite connected leg chains, loop endpoints, compensated horizontal stance motion, unchanged accepted surfaces/bind pose and active attack-hand reach. Source previews and the later rendered game review are separate: numeric checks alone do not establish foot planting, six-weapon hand stability, attack readability or naturalness.

Regenerate from the repository root with Blender 5.1.2 `--background --python Scripts/create_candidate05_motion.py`. Then run the existing metadata sanitizer on only these 25 FBXs and two blends, and refresh inventory hashes after sanitation before the coordinated import. Generated source-path headers are publication metadata, not geometry. No third-party animation, mesh, audio or Project Zero assets are used.
