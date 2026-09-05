"""Read-only Blender FBX roundtrip geometry/axis audit; no source mutation."""
from pathlib import Path
import bpy,json
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
audit={}
for filename in ['SM_Carbine','SM_FloorModule','SM_WallBay','SK_Response','SK_Infected']:
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.context.scene.unit_settings.system='METRIC'
    bpy.context.scene.unit_settings.scale_length=.01
    bpy.ops.import_scene.fbx(filepath=str(ROOT/'ArtSource'/'Exports'/(filename+'.fbx')),use_anim=False)
    objects=list(bpy.context.scene.objects)
    meshes=[o for o in objects if o.type=='MESH']
    points=[o.matrix_world@Vector(p) for o in meshes for p in o.bound_box]
    lo=[min(p[i] for p in points) for i in range(3)]
    hi=[max(p[i] for p in points) for i in range(3)]
    row={'min_cm':lo,'max_cm':hi,'dimensions_cm':[hi[i]-lo[i] for i in range(3)],'mesh_count':len(meshes)}
    rigs=[o for o in objects if o.type=='ARMATURE']
    if rigs:
        rig=rigs[0]
        row['bones']={b.name:{'head_cm':list(rig.matrix_world@b.head_local),'tail_cm':list(rig.matrix_world@b.tail_local)} for b in rig.data.bones if b.name in ('root','pelvis','head','weapon_r','hand_r','hand_l')}
    audit[filename]=row
(ROOT/'Evidence'/'fbx_blender_roundtrip.json').write_text(json.dumps(audit,indent=2))
print(json.dumps(audit,indent=2))
