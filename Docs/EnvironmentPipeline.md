# Environment and firearm candidate

All thirteen static meshes, their material palette, and the blood alpha mask are original to Project ONE. `Scripts/create_environment.py` authored them with Blender 5.1.2; no Project Zero or third-party assets were inputs. Editable source is `ArtSource/Environment/ProjectONE_IndustrialKit.blend`; `manifest.json` records the palette, exports and axes. The saved Blender file arranges models as a catalog; each FBX is exported at its intended local origin before catalog placement.

| Element | Authored details |
| --- | --- |
| Architecture | Sealed floor modules and joints, paneled structural bays, camera-side barriers, pressure door with leaf seals and observation panels |
| Equipment | Diagnostic console and keys, removable power modules and fins, pressure vessel with pipework/handwheel/gauge, low research benches and sealed case |
| Identity | Containment sign, service hatches, paired light strips; graphite/teal/pale metal and restrained amber |
| Carbine | Grip-origin receiver, stock/buffer, trigger/guard, magazine, handguard, continuous rail, optic, barrel and ported muzzle |
| Blood | Deterministic irregular RGBA alpha with satellite droplets in `ArtSource/Textures/T_BloodMask.png` |

The modeling helpers use meters and store centimeters, with Blender scene unit scale 0.01. FBX exports use `axis_forward=-Y`, `axis_up=Z`, unit conversion enabled and scale 1. The firearm points +X, has a grip-center origin, and ends at approximately (59,0,14)cm. The importer uses explicit legacy FBX options and creates native Unreal materials from the authored palette instead of relying on FBX shader translation.

The actual Unreal audit found that `force_front_x_axis=True` rotates these exports onto -Y. The verified setting is **False**, which preserves +X and converts Blender Y to Unreal -Y. Reimports also need the existing asset's stored `asset_import_data` flag updated; task options alone do not change it. The final audit measured carbine extents (49.65,4.425,19.13)cm, wall extents (200,25.25,150)cm, player height 180.6cm, and the expected hand positions. `Evidence/unreal_asset_audit.json` contains the actual imported results and all 15 clip durations. `Scripts/inspect_import.py` reproduces that read-only audit.

`Scripts/import_assets.py` imports all original environment and character exports, animations, the alpha mask, and original audio; it writes `Evidence/asset_import_log.txt`. `Scripts/create_map.py` builds `/Game/ONE/Maps/Containment`, a 24×20m arena with walkable ground at Z=0, a +Y camera cutaway, two low central obstacles, and four spawn points. Full side/back architecture and equipment remain outside the central routes. It validates the navigation volume's actual bounds before saving. The map uses movable lighting and fixed exposure; runtime inspection determines whether this initial light balance needs further tuning.

Run generation with the installed Blender executable and `--background --python Scripts/create_environment.py`. After compiling `ProjectONEEditor`, run `Scripts/import_assets.py` in the **full Unreal Editor** with `-ExecutePythonScript="<absolute project path>/Scripts/import_assets.py"`. The installed UE 5.7 Python commandlet does not initialize `StaticMeshEditorSubsystem`, which the current importer requires for deterministic environment collision. Map creation remains a separate controlled regeneration of the authored map; use the targeted collision repair below when retaining the existing layout.

For separate audio-import commandlets, include `-AllowCommandletAudio` so the Bink decoder is initialized when Unreal builds sound wave assets. Close other Project ONE Unreal processes during import; asset readers can otherwise hold a Windows file lock during saving. The historical Candidate01 import and map-generation commandlets returned exit code 0; that does not remove the current full-editor requirement above. Remaining import warnings concern missing UVs on the detached arm and nearly-zero tangents on some lettering/details; these materials use vertex colors without normal-map sampling. The historical map load reported no Recast instance before runtime initialization. Current navigation changes require a completed build and separate runtime pursuit checks.

The first actual gameplay images exposed a serious lighting error: the intended fixed exposure did not take effect with the project's default exposure disabled, and unspecified point-light units produced excessive illumination. The next candidate explicitly sets manual metering, disables physical camera exposure, applies -4.7 stops compensation, and sets fill/safety lights to 8500/1800 lumens. A new original floor-wayfinding mesh adds a restrained safety-zone outline, amber cues and a B-07 stencil to the central camera view. These are responses to actual in-engine inspection, with subsequent runtime captures needed to judge the revision.

An additional persisted-map audit caught an Unreal Python constructor error: positional Rotator arguments tilted architecture and inverted the floor, burying shallow markings. All map Rotators now use explicit `pitch`, `yaw`, and `roll` keywords. Numeric assertions verify 30 level floor modules with top Z=1.25cm, full walls with top Z=300cm, and the full wayfinding bounds above the floor. This correction changes the actual obstacle geometry, so earlier navigation/performance measurements must be superseded by another runtime test. `Scripts/inspect_map.py` preserves these regression checks.

Candidate01 integration checks passed: a blocking collision trace measured the floor at Z=1.35cm including the simple-collision margin; final garment meshes retained authored vertex colors and mapped material slots. `Scripts/Audit-SourceSync.ps1` independently compared all 39 original FBX/PNG/WAV source MD5 values with Unreal's serialized `FAssetImportInfo.FileMD5` metadata. Every source matched its imported asset; `Evidence/final_source_sync.json` records that historical snapshot. The map was not regenerated during that final targeted garment import.

`Evidence/Blender_Carbine_Inspection.png` was rendered and visually inspected to check the newly authored weapon silhouette and its assembled detail. It is a Blender modeling check, **not gameplay or in-engine visual evidence**. Runtime animation, grip alignment, collisions, lighting and final scene quality must be reported from the engine separately. This is an initial art candidate for review, not approved production art.

## Candidate03 Stage D collision and navigation repair

The [original collision audit](../Evidence/Candidate03/StageD/EnvironmentCollisionBefore.json)
found seven convex hulls per inspected mesh. Reimport had retained duplicate
collision and older hulls rotated by 90 degrees around Z. Four of the seven
hulls on each non-floor asset extended beyond its current render bounds; the
square floor's seven hulls coincided. On `SM_WallBay`, for example, the stale
hulls extended to roughly ±200 cm along local Y instead of the current wall's
approximately -15 to +35.5 cm. These invisible obstructions fragmented paths
through the unchanged arena.

The targeted [repair script](../Scripts/repair_candidate03_environment_collision.py)
handles exactly these eight existing assets:

| Architecture | Equipment |
| --- | --- |
| `SM_FloorModule` | `SM_PowerRack` |
| `SM_WallBay` | `SM_PressureVessel` |
| `SM_CutawayBarrier` | `SM_LabConsole` |
| `SM_PressureDoor` | `SM_ResearchBench` |

It loads the existing `Containment` map, removes each asset's previous collision,
generates one fresh NDOP18 convex hull from current render geometry, and saves
the mesh. It asserts one convex hull and no primitive shapes, then reads back
the simple and navigation collision vertices. The
[repair report](../Evidence/Candidate03/StageD/EnvironmentCollisionRepair.json)
records zero collision vertices outside the 2 cm render-bounds tolerance and
unchanged render bounds within 0.001 cm. Actor positions, mesh geometry,
materials and the visible layout remain exactly the same. The report's stored
before/after pair is a later rerun with one hull on both sides; use the separate
original audit to inspect the seven-hull failure.

Run the targeted script in the full Editor with
`-ExecutePythonScript="<absolute project path>/Scripts/repair_candidate03_environment_collision.py"`
after compiling the editor module. `StaticMeshEditorSubsystem` must exist;
`UnrealEditor-Cmd -run=pythonscript` is insufficient in this installation. The
script sets the saved Recast agent radius/height from **35/144 to 32/185 cm**,
calls `rebuild_navigation_and_wait`, asserts completion and only then saves the
existing level. Finishing mesh imports alone is not a completed navigation build.

The [before/after graph comparison](../Evidence/Candidate03/StageD/navigation_comparison.md)
records six floor components becoming one, separate elevated components changing
from two to zero, and main connected floor area increasing from 339.53855 to
387.51545 m². Original corner, rack and bench target positions now share the
main floor component. Both collision and serialized agent settings changed, so
the comparison does not isolate their individual effects. A separate headless
phases 6–10 run completed 173 checks with zero failures, including 18/18 enemies
progressing over 100 cm and actual attack contact in each corner/rack/bench
fixture. Navigation plots visualize captured Recast data; they are not gameplay
screenshots or a manual playtest.

To prevent recurrence, [the original importer](../Scripts/import_assets.py) now
clears previous collision for these same eight environment meshes on every
import, regenerates NDOP18, and rejects any result other than one convex hull
and zero primitive shapes. It fails explicitly if the full-editor subsystem is
unavailable. Original FBX axes remain `force_front_x_axis=False`; rebuilding
collision prevents older orientations from surviving future reimports.

Stage D rendered physicality gates remain pending. Successful hull readback and
headless navigation do not establish natural corpse settling, readable pools,
all detached-part contacts or user acceptance. Agent-run captures and automation
remain distinct from the user's own playtest; no final user playtest pass is
claimed here.
