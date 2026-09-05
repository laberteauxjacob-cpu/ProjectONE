# Candidate02 weapon sources

All geometry, animation and sound design here is original Project ONE work. The
accepted Candidate01 character meshes, rigs, carbine source and old clips remain
unchanged. The new carbine meshes split its existing surface into body and magazine.

`ResponseWeaponActions.blend` contains the accepted player and its retained
reference actions, eight new actions, the new shotgun and driven moving props.
`InfectedHeavyReaction.blend` adds one restrained planted hit action on the
accepted infected rig. The three PNGs are Blender source checks, not gameplay evidence.
Open the Action Editor and select the named actions to inspect or edit them.
The rig's `pump_travel`, `shell_visible`, `weapon_kind`, and `carbine_mag_state`
properties drive preview props. Runtime uses the same curves and timed events.

| New asset | Duration / purpose |
|---|---|
| A_Response_ShotgunReady | 2.40 s loop; upper body over the accepted gait |
| A_Response_ShotgunFire | 0.22 s recoil |
| A_Response_ShotgunPump | 0.56 s; eject 0.18, reverse 0.21, lock 0.44 |
| A_Response_ShotgunReloadStart | 0.35 s into loading posture |
| A_Response_ShotgunReloadShell | 0.90 s; held shell visible 0.12–0.60, earned insert 0.60 |
| A_Response_ShotgunReloadEnd | 0.32 s return to grip |
| A_Response_Equip | 0.36 s; visual/audio swap 0.18 |
| A_Response_CarbineReload | 2.10 s; magazine out 0.40, seated 1.20, bolt 1.74 |
| A_Infected_HeavyHit | 0.52 s planted torso reaction |

Animation exports sample at 100 Hz. The inherited rig has 21 authored Blender
bones and the additional armature root retained by Unreal, matching the accepted
22-bone runtime skeleton. These are baked skeletal clips, not runtime limb rotations.

All five static meshes use the weapon grip as origin and centimetres, +X barrel,
+Z up. The shotgun muzzle is `(64.5, 0, 14)`. The fore-end is centred at
`(18, 0, 6)` and translates `-9 X` cm at full travel. Its smooth curve is
`(0,0), (0.21,1), (0.44,0), (0.56,0)`. The shell cylinder uses local X, with
its origin at its centre. Exact runtime attachment offsets and handedness-adjusted
ejection position are in `inventory.json`. The accepted anatomical naming reflection
at FBX import is preserved consistently across both candidates.

`../../Audio/Candidate02` contains 25 original 48 kHz mono PCM sound candidates:
three variations per weapon shot, pump/reload/equip/empty mechanisms, and flesh,
concrete and metal impacts. Source code builds them from deterministic noise,
filters, envelopes and damped resonances; there are no downloaded samples or packs.
Shot peaks are −3 dBFS; mechanism and impact peaks range from −10 to −17 dBFS.
Per-file measured peak, RMS, duration and event role are in its inventory.
These are synthesized candidates; source checks do not establish perceptual
quality or a recorded-real-weapon sound. Gameplay listening and event synchrony
must be assessed in the actual build.

Regenerate with Blender 5.1.2 using `Scripts/create_candidate02_weapon_assets.py`.
The targeted `Scripts/import_candidate02_weapon_assets.py` imports only the new
inventory and three new materials, reuses the accepted materials/skeletons, and
checks imported clip lengths. It writes its results under ignored `Saved/Candidate02`.
Run the portable metadata sanitizer after regeneration and before publication.
