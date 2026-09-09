# Candidate06 original pickup emblems

Original geometry in `Source/ProjectONE/ONE06PickupVisualComponent.cpp` defines
an orange/red skull, gold multiplication mark with a numeral 2, and teal
ammunition crate. Their different silhouettes have matching Canvas icons in
`ONEHUD.cpp`. They reuse Unreal's engine cube/sphere/cylinder assets; no external
art, texture, model, font or Project Zero input is introduced. `design.json`
records the editable inventory.

Root-scheduled `Scripts/create_candidate06_pickup_art.py` authors one depth-tested
parameter material and writes actual source/hash/import evidence. The reusable
visual component uses no map coordinates, at most 12 mesh parts, three dynamic
materials, one 64 cm nonshadow light and no particle emitters. The gameplay actor
alone controls collection and lifetime. Visual meshes have no collision or
navigation effects.

The unlit material is tuned for the two authored maps' manual exposure bias of
-4.7 stops. A named 32x emission gain gives coloured faces 20.8 emission and glow
parts 64 before the warning pulse; dark cutouts retain 0.15 emission. This is a
runtime parameter change using the existing material asset. It responds to an
actual early-lifetime capture where the world crate appeared nearly black.

The same point light now explicitly uses 32 lumens and sits 48 cm below the
hovering emblem. With ordinary placement at navigation floor +45 cm, it is
22.5-27.5 cm above that floor, or at most 31.34 cm including the warning scale.
Its unchanged 64 cm influence radius can therefore reach nearby ground. The
previous position was about 83 cm above the floor, outside that radius. Mesh,
material and light counts remain unchanged. These values require actual image
review and do not establish visual acceptance by themselves.

Motion is a 2.5 cm hover with 12-degree yaw sway. The final five seconds have a
restrained 1.3 Hz glow warning and a final 0.85-second opacity fade. Collection
hides geometry and requests one dedicated original stylized cue through the
bounded Action concurrency group. Source specifications and successful imports
do not constitute in-engine visual or auditory acceptance.

`ONE06ImpactSubsystem.cpp` separately reuses the original `M_EyeDark` material
and one instanced cylinder mesh for actual solid-surface bullet/pellet marks.
The 0.8/0.6 cm radius discs keep the supplied collision point/normal, reuse a
256-instance cap, expire after 20 gameplay seconds, and have no collision,
navigation or shadows. They add no external texture or invented victim hits.
