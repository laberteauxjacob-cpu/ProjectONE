# Candidate04 weapon sources

These are original Project ONE models and upper-body actions. The six complete
assemblies are M1911 / Last Word, M4A1 / Overcurrent, and Remington 870 /
Gravebreaker. The accepted character mesh, 22-bone runtime skeleton, Candidate03
locomotion, and previous weapon assets remain unchanged.

`WeaponWorkshop.blend` contains editable assemblies with separate body,
magazine, slide or fore-end as appropriate. `ResponseWeaponActions.blend` adds
the accepted Response mesh/rig and twelve baked actions, with driven preview
props. The four `Blender_*.png` files show source inspection only; they are not
engine screenshots or proof of runtime integration. `inventory.json` is the
portable asset, material, transform and timing contract.

| Family | Geometry and original upgrade treatment |
| --- | --- |
| M1911 / Last Word | Short single-stack frame, angled grip panels, separate serrated slide, open ejection cut, exposed hammer, fixed sights, magazine and short open-mouth brass. Last Word adds violet receiver/slide inlays, selected grip charge marks and a matching magazine floorplate. |
| M4A1 / Overcurrent | Forged receiver silhouette, shorter quad handguard, A-frame front sight, carry handle, adjustable stock, side controls and curved stamped magazine. Overcurrent adds cyan receiver/handguard channels and restrained stock/magazine marks. |
| Remington 870 / Gravebreaker | Rounded receiver, conventional stock, tube magazine, open muzzle, corn-cob fore-end with twin action bars, side cross-bolt controls and preserved loading contact. Gravebreaker adds ember receiver cells, stock seam and moving fore-end inlay. |

The models use original profiles, bevelled parts and vertex materials. No logos,
external meshes, scanned textures or commercial-game assets are used. Shape
references were the manufacturer's [M1911A1 manual](https://www.colt.com/wp-content/uploads/2023/02/wwiireproductionpistolmodelm1911a1.pdf),
[Colt M4 catalogue, page 24](https://www.colt.com/wp-content/uploads/2024/08/2024-Colt-MLE-Catalogweb.pdf)
and [RemArms Model 870](https://www.remarms.com/shotguns/pump-action/model-870/).
These guide recognisable form; game capacities and cadence are independent balance
values. The experimental variants are original fictional treatments.

All static parts use centimetres, +X barrel and +Z up. Body, seated magazine,
slide and fore-end share one grip-centred origin, so a complete machine preview
attaches every listed assembly part at identity. Runtime retains the accepted
`weapon_r` component-space reference quaternion inverse. Legacy FBX import
reflects source Y; the inventory's runtime offsets already account for that.
The pistol spent case has its own centred origin and local +X cylinder axis.

| Runtime contact | M1911 / Last Word | M4A1 / Overcurrent | 870 / Gravebreaker |
| --- | --- | --- | --- |
| Muzzle, cm | `(16.5,0,5.5)` | `(54.5,0,14)` | `(64.5,0,14)` |
| Ejection, cm | `(5.7,-1.5,5.5)` | `(3,-3.7,14)` | `(5,-4.5,13.5)` |
| Fresh magazine offset in bind-cancelled support-hand axes | `(0.5,0,1.8)` | `(-10.5,0,0)` | No detachable magazine |

Drop the old magazine from the evaluated **seated magazine component transform**
at the removal event, before hiding that component. The fresh handled prop uses
the different support-hand offset above. This keeps the dropped old magazine
independent of the later replacement. No animation clip grants ammunition by
itself: the operation event owns that transaction.

All new actions use prefix `A_Response_C04_`, are sampled at 100 Hz and import to
`/Game/ONE/Animations/Candidate04`. They layer from `spine_01`; lower-body gait
selection and timing remain in the accepted runtime graph.

| Action | Duration | Events / role |
| --- | --- | --- |
| PistolReady | 2.40 s | Two-handed pistol stance; short breathing loop |
| PistolFire | 0.18 s | Wrist/arm recoil; slide travel `0:0, .025:1, .07:0, .18:0`, full travel `-3X` cm |
| PistolReload | 1.80 s | Old magazine removal/drop .28; fresh prop visible .64; seat/ammunition 1.10; slide release 1.40 |
| PistolEquip | 0.36 s | Lower/raise with swap .18 |
| CarbineReload | 2.10 s | Old magazine removal/drop .40; fresh prop .74; seat/ammunition 1.20; bolt 1.74 |
| UnarmedReady | 2.40 s | Hands lowered, upper-body-only fallback |
| PistolHandoff / CarbineHandoff / ShotgunHandoff | 0.72 s each | Present/lower the actual grip; transfer .48 |
| PistolRetrieve / CarbineRetrieve / ShotgunRetrieve | 0.64 s each | Reach .18, then bring the actual grip into the family ready pose |

The pistol reload source previews an empty-start slide. Runtime must keep the
slide forward on a tactical reload and only release a retained rearward slide
when appropriate. The supported arcade accounting does not model a separate
chamber. The source hands contain the accepted fixed finger geometry; no new
finger-bone articulation is claimed.

The 870 retains Candidate02 fire/pump/single-shell actions and exact fore-end
contact `(18,0,6)`. Full pump travel is `-9X` cm, with source curve
`0:0, .21:1, .44:0, .56:0`; ejection occurs at pump .18. Its loading gate
retains the accepted source height. Faster upgrades divide fire/pump durations
and their events by 1.15 and sample the matching source action proportionally;
they do not accelerate locomotion or all reloads.

At machine transfer, sample the evaluated gun transform and interpolate the
complete model into the cradle. At retrieval, interpolate from the actual
output to the evaluated grip. Do not teleport the player or scale a long gun
into a pistol-sized cradle. The source handoff contains only the player pose;
the machine transaction and preview movement belong to the runtime.

Regenerate with Blender 5.1.2 and `Scripts/create_candidate04_weapon_assets.py`.
Use `--no-render` after `--` only when source images are not being refreshed.
Run `Scripts/sanitize_asset_metadata.py` on the new FBX, Blender and PNG files
before publication and final import. Root coordinates the Unreal window for
`Scripts/import_candidate04_weapon_assets.py`, which creates only the new
inventory materials/meshes/actions, reuses the accepted skeleton and checks
imported bounds, timing and material assignment. Its report is stored under
ignored `Saved/Candidate04`; source checks alone are not a runtime pass.
