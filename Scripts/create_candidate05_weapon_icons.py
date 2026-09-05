"""Blender5.1 background-only original assembled weapon images for the C05 HUD.
Uses accepted C04 original body/magazine/slide/fore-end objects at shared grip
origin. No source mesh/material is rewritten. Never connects to a GUI process.
"""
import bpy
import hashlib
import json
from mathutils import Vector,Matrix
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/UI/Candidate05'
SOURCE=ROOT/'ArtSource/Weapons/Candidate04/WeaponWorkshop.blend'
catalog=json.loads((ROOT/'ArtSource/Weapons/Candidate04/inventory.json').read_text())
assemblies=catalog['assemblies']; names=[name for group in assemblies.values() for name in group]
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene; scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=.01
OUT.mkdir(parents=True,exist_ok=True)
with bpy.data.libraries.load(str(SOURCE),link=False) as (available,data):
    assert all(name in available.objects for name in names); data.objects=names
objects={obj.name:obj for obj in data.objects}
for obj in objects.values():
    scene.collection.objects.link(obj); obj.parent=None; obj.matrix_world=Matrix.Identity(4); obj.hide_render=True
world=bpy.data.worlds.new('Original UI neutral light'); world.use_nodes=True
world.node_tree.nodes['Background'].inputs['Color'].default_value=(.15,.18,.22,1)
world.node_tree.nodes['Background'].inputs['Strength'].default_value=.4; scene.world=world
for label,position,power,size in [('Key',(25,-120,140),240000,90),('Rim',(-60,90,90),210000,75),('Fill',(100,-60,35),95000,70)]:
    light=bpy.data.lights.new(label,'AREA'); light.energy=power; light.shape='DISK'; light.size=size
    obj=bpy.data.objects.new(label,light); scene.collection.objects.link(obj); obj.location=position
    obj.rotation_euler=(Vector((10,0,5))-obj.location).to_track_quat('-Z','Y').to_euler()
camera_data=bpy.data.cameras.new('Weapon icon camera'); camera=bpy.data.objects.new('Weapon icon camera',camera_data)
scene.collection.objects.link(camera); scene.camera=camera; camera_data.type='ORTHO'; camera_data.clip_end=2000
scene.render.engine='CYCLES'; scene.cycles.samples=20; scene.cycles.use_denoising=True
scene.render.resolution_x=1024; scene.render.resolution_y=384; scene.render.resolution_percentage=100
scene.render.film_transparent=True; scene.render.image_settings.file_format='PNG'; scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='AgX'
manifest={'candidate':'05','generator':'Scripts/create_candidate05_weapon_icons.py',
    'source_blend':'ArtSource/Weapons/Candidate04/WeaponWorkshop.blend',
    'source_blend_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
    'provenance':'Actual original assembled Project ONE weapon meshes/materials, camera-only derived render; no unrelated silhouette or third-party imagery.',
    'images':{}}
for family,parts in assemblies.items():
    for obj in objects.values(): obj.hide_render=obj.name not in parts
    bpy.context.view_layer.update()
    bounds=[objects[name].matrix_world@Vector(corner) for name in parts for corner in objects[name].bound_box]
    lo=Vector(tuple(min(p[i] for p in bounds) for i in range(3))); hi=Vector(tuple(max(p[i] for p in bounds) for i in range(3)))
    center=(lo+hi)*.5; camera.location=center+Vector((20,-220,45))
    camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler()
    inverse=camera.rotation_euler.to_matrix().transposed(); projected=[inverse@(p-center) for p in bounds]
    width=max(p.x for p in projected)-min(p.x for p in projected); height=max(p.y for p in projected)-min(p.y for p in projected)
    camera_data.ortho_scale=max(width,height*1024/384)*1.12
    filename='T_UI05_'+family+'.png'; scene.render.filepath=str(OUT/filename)
    bpy.ops.render.render(write_still=True)
    path=OUT/filename
    manifest['images'][family]={'source':path.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/UI/Candidate05/'+path.stem,
        'parts':parts,'width':1024,'height':384,'source_bounds_cm':[list(lo),list(hi)],'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
scene.render.filepath='//T_UI05_Gravebreaker.png'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WeaponIconStudio.blend'))
(OUT/'weapon_icons.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('CANDIDATE05_WEAPON_ICONS_RENDERED 6 actual assembled weapons')
