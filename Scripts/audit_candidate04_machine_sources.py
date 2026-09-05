"""Read-only Blender roundtrip of all original C04 machine FBXs and source rig.

Run in a separate Blender --background --disable-autoexec process. It does not
operate on an existing interactive Blender window or modify source assets.
"""
from pathlib import Path
import bpy
import hashlib
import json
import math
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
inventory=json.loads((ROOT/'ArtSource/Machines/Candidate04/inventory.json').read_text())
rows=[]
for name,row in inventory['static_meshes'].items():
    source=ROOT/row['source'];digest=hashlib.sha256(source.read_bytes()).hexdigest()
    assert digest==row['source_sha256'],'Source digest changed: '+name
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
    bpy.ops.import_scene.fbx(filepath=str(source),use_image_search=False)
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    assert len(meshes)==1,'Expected exactly one assembled export: '+name
    o=meshes[0];o.data.calc_loop_triangles()
    assert len(o.data.loop_triangles)==row['triangles'],'Triangle identity changed: '+name
    coords=[o.matrix_world@v.co for v in o.data.vertices]
    actual={'min':[min(v[i] for v in coords) for i in range(3)],'max':[max(v[i] for v in coords) for i in range(3)]}
    error=max(abs(actual[side][i]-row['source_bounds_local_cm'][side][i]) for side in ('min','max') for i in range(3))
    assert error<.025,'Axis/pivot/dimension roundtrip mismatch: '+name+str(actual)
    assert set(m.name for m in o.data.materials)==set(row['materials']),'Materials changed: '+name
    color=o.data.color_attributes.get('Color');assert color and len(color.data)>0,'Missing authored vertex colors: '+name
    assert all(math.isfinite(v) for point in coords for v in point),'Nonfinite mesh geometry'
    rows.append({'mesh':name,'source':row['source'],'source_sha256':digest,'triangle_count':len(o.data.loop_triangles),
      'roundtrip_bounds_cm':actual,'max_bounds_error_cm':error,'vertex_color_entries':len(color.data),'result':'PASS'})

source=ROOT/inventory['source'];source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
bpy.ops.wm.open_mainfile(filepath=str(source),load_ui=False)
scene=bpy.context.scene;lid=bpy.data.objects['SM_C04_BoxLid'];cradle=bpy.data.objects['SM_C04_UpgradeCradle']
pose_rows=[]
for frame in (1,12,25,36,55,75,180,215,231,270):
    scene.frame_set(frame)
    pose_rows.append({'frame':frame,'lid_pitch_source_degrees':math.degrees(lid.rotation_euler.y),
      'cradle_source_location_cm':list(cradle.location)})
scene.frame_set(1);assert abs(lid.rotation_euler.y)<.001 and abs(cradle.location.x-94)<.001
scene.frame_set(75);assert abs(math.degrees(lid.rotation_euler.y)+108)<.01 and abs(cradle.location.x+5)<.001
scene.frame_set(270);assert abs(cradle.location.x-94)<.001
report={'candidate':'04','status':'PASS','source_blend':inventory['source'],'source_blend_sha256':source_hash,
 'mesh_count':len(rows),'meshes':rows,'source_animation_samples':pose_rows,
 'scope':'Blender FBX roundtrip dimensions/pivots, triangle counts, material slots, vertex colors and source assembly keyframe poses. Unreal/runtime appearance and sounds are separate checks.'}
out=ROOT/'Saved/Candidate04/MachineSourceValidation.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n')
print('CANDIDATE04_MACHINE_SOURCE_PASS',len(rows),'FBX roundtrips and source lid/cradle poses')
