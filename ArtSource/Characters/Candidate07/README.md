# Candidate07 infected source

The trio consists of original Project ONE geometry on the existing infected bind rig. Each appearance has one editable Blender file, five modular FBXs, a design file, an inventory and a source-validation report.

| Appearance | Vertices / triangles | Distinct modeled features |
| --- | --- | --- |
| Maintenance | 31,205 / 60,406 | Damaged blue jacket, uneven sleeve, gaunt face and receding swept hair |
| Laboratory | 33,409 / 64,798 | Lined split coat tails, broad lapels, low pockets, narrower mirrored facial structure, opposite cheek wound and grey side/back hair |
| FacilityStaff | 33,514 / 64,982 | Slimmer button shirt, rolled cuffs, loose tie, belt, brown trousers, broader lower face and broken dark crown hair |

The final neutral, face, side, flex and separated-part source images for both new appearances were inspected. The laboratory hip clipping and staff crown intersection found in their first previews were corrected before import. Source stills do not establish ordinary-camera readability or motion quality; those require the separate engine and packaged reviews. The first Maintenance gameplay checkpoint is documented in [the pass report](../../../Docs/Passes/Candidate07.md).

All three use 21 authored bones (the imported skeleton also contains the armature root), unchanged bind matrices and the same five parts: Core, Head, ArmLeft, ArmRight and LegLeft. Source `_r` is anatomical left; Unreal reflects source Y. Arm cuts remain 4.8 cm along the upper arm, the supported thigh cut remains at 38%, and the neck rim remains at Z 157.8 cm. Each appearance passed 140 evaluated seam samples and normalized-weight checks. Hair follows Head; the laboratory coat tails end above the thigh cut and stay on Core, with separate proximal-thigh weights. There is no cloth bridge into a removed leg.

The shared head query is centered at (-0.2, 0, 168.7) cm, radius 9 cm and half-height 12.3 cm. Both new heads' 8,031 actual skin vertices fit this bind-pose envelope with at least 0.0869 cm margin; cosmetic hair is excluded. [physics_fit.json](physics_fit.json) records the Maintenance measurement and smaller simulation approximation. New appearances reuse the five C07 physics assets; runtime posed/physical fit is a separate check. The appearance definitions change no statistics: health 112, movement 100/195 cm/s and strike damage 19 remain shared.

## Authoring and import

From the repository root, use Blender 5.1 with `Scripts/create_candidate07_infected.py` and `--python-exit-code 1`. After the Blender argument separator, select `--variant maintenance`, `--variant laboratory` or `--variant facility_staff`; add `--render` for the five inspection images. Each run writes only that appearance's source/export paths. The two new variants reuse the existing original pore/weave PNGs and enforce Maintenance, player and earlier-source hash guards.

Before importing, run `Scripts/sanitize_asset_metadata.py` on the changed C07 Blender/FBX/PNG files. Retain its proof that art data is unchanged, then run `Scripts/refresh_candidate07_metadata.py` with that exact applied report. Do not refresh hashes to excuse an unreviewed geometry change. Compile the native C07 definition/helper before the coordinated Unreal Python import.

`Scripts/import_candidate07_infected.py` selects `PROJECTONE_C07_VARIANT=maintenance`, `laboratory` or `facility_staff`; the default remains Maintenance. It verifies source hashes, preserves the skeleton reference pose, imports LINEAR vertex RGB/roughness, verifies skeletal material usage/connections and creates `DA_Infected_Maintenance`, `DA_Infected_Laboratory` or `DA_Infected_FacilityStaff`. New variants use their own materials and reuse the existing normal textures/physics assets without resaving those shared assets. This script does not alter player assets.

## Motion source

`Scripts/create_candidate07_infected_motion.py` authors all 14 clips from Maintenance by default. `--recoveries-only` retains the other 12 actions/FBXs; `--supine-only` retains 13, including the working 2.55-second prone recovery. Supine remains 3.0 seconds and measures deformed garment/head/boot floor support during its roll. Both partial modes verify retained FBX hashes and evaluated poses, set action fake users, reopen the saved blend and verify all 14 bound actions. An explicit `--retained-source` is only for a known preserved source with matching inventory.

Sanitize changed motion sources, refresh their proven metadata hashes, then use `Scripts/import_candidate07_infected_motion.py` to populate the 14 compatible clips on the variant definitions. Runtime holds the first authored pose during its initial snapshot blend and then plays the clip. Editable source, numeric invariants, import success, chronological visual review, perceptual audio, native play and packaged results remain separate evidence scopes.
