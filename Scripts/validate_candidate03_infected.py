"""Blender source/FBX checks and optional labelled source previews, not UE QA.

blender --background --python Scripts/validate_candidate03_infected.py -- --render
Run only after the generator is finished, before final metadata sanitation.
"""
from pathlib import Path
import bpy,json,math,sys
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/Characters/Candidate03'
manifest=json.loads((SOURCE/'infected_inventory.json').read_text())
source_path=ROOT/manifest['editable_source']
bpy.ops.wm.open_mainfile(filepath=str(source_path))
rig=bpy.data.objects['Rig_Infected']
rig.animation_data_clear()
for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4)
bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
expected={}
for name in manifest['meshes']:
    o=bpy.data.objects[name]
    expected[name]={'bounds':[[min(v.co[a] for v in o.data.vertices) for a in range(3)],[max(v.co[a] for v in o.data.vertices) for a in range(3)]],
        'bones':{b.name:list(b.head_local) for b in rig.data.bones},'groups':{g.name for g in o.vertex_groups}}
report={'result':'PASS','scope':'Blender FBX roundtrip/source preview, not gameplay evidence','meshes':{}}
for name,definition in manifest['meshes'].items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
    bpy.ops.import_scene.fbx(filepath=str(ROOT/definition['source']),automatic_bone_orientation=False)
    imported=next(o for o in bpy.data.objects if o.type=='MESH')
    arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    coords=[imported.matrix_world@v.co for v in imported.data.vertices]
    bounds=[[min(v[a] for v in coords) for a in range(3)],[max(v[a] for v in coords) for a in range(3)]]
    error=max(abs(bounds[i][j]-expected[name]['bounds'][i][j]) for i in range(2) for j in range(3))
    heads={b.name:arm.matrix_world@b.head_local for b in arm.data.bones}
    assert set(heads)==set(expected[name]['bones']),'Roundtrip bone names changed'
    bone_error=max((heads[k]-Vector(v)).length for k,v in expected[name]['bones'].items())
    assert error<.005 and bone_error<.005,(name,error,bone_error)
    assert imported.data.color_attributes,'Vertex paint missing from FBX'
    report['meshes'][name]={'source_export':definition['source'],'bounds_cm':bounds,
        'max_bounds_error_cm':error,'max_bone_head_error_cm':bone_error,'rig_bones':len(heads),
        'imported_vertices':len(imported.data.vertices),'vertex_color_layers':[a.name for a in imported.data.color_attributes]}

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=.01
clip_name=next(iter(manifest['clips']));clip_def=manifest['clips'][clip_name]
bpy.ops.import_scene.fbx(filepath=str(ROOT/clip_def['source']),automatic_bone_orientation=False)
arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
action=arm.animation_data.action
assert action
start,end=action.frame_range;fps=bpy.context.scene.render.fps/bpy.context.scene.render.fps_base
duration=(end-start)/fps
assert abs(duration-1)<.001,('Animation duration',duration,start,end,fps)
value=start+fps*.48;frame=math.floor(value);bpy.context.scene.frame_set(frame,subframe=value-frame)
bpy.context.view_layer.update()
active=arm.matrix_world@arm.pose.bones['hand_l'].head
quiet=arm.matrix_world@arm.pose.bones['hand_r'].head
assert active.x>45 and active.x-quiet.x>25
report['animation']={'export':clip_def['source'],'duration_seconds':duration,'fps':fps,
    'contact_seconds':.48,'right_hand_UE_anatomy_source_position_cm':list(active),'other_hand_position_cm':list(quiet)}
(SOURCE/'infected_roundtrip.json').write_text(json.dumps(report,indent=2)+'\n')
print('C03_INFECTED_ROUNDTRIP_PASS',json.dumps({'meshes':5,'duration':duration}))

if '--render' in sys.argv:
    bpy.ops.wm.open_mainfile(filepath=str(source_path))
    scene=bpy.context.scene;rig=bpy.data.objects['Rig_Infected'];rig.animation_data_clear()
    for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4)
    scene.frame_set(1)
    scene.render.engine='CYCLES';scene.cycles.samples=24
    scene.render.threads_mode='FIXED';scene.render.threads=8
    scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
    scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.10,.12,.14,1)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.55
    scene.view_settings.view_transform='AgX'
    camera_data=bpy.data.cameras.new('Source_validation_camera');camera=bpy.data.objects.new('Source_validation_camera',camera_data)
    scene.collection.objects.link(camera);scene.camera=camera
    camera.location=(330,-490,315);camera.rotation_euler=(Vector((0,0,102))-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type='ORTHO';camera.data.ortho_scale=270
    for name,location,energy,size in [('Key',(200,-260,360),1700,220),('Fill',(100,240,220),1250,180),('Rim',(-150,50,270),1600,170)]:
        light_data=bpy.data.lights.new(name,'AREA');light_data.energy=energy*300;light_data.shape='DISK';light_data.size=size
        light=bpy.data.objects.new(name,light_data);scene.collection.objects.link(light);light.location=location
        light.rotation_euler=(Vector((0,0,100))-light.location).to_track_quat('-Z','Y').to_euler()
    for label,offsets in [('Intact',{}),('Caps',{'SK_Infected_Head':(0,0,38),'SK_Infected_ArmLeft':(0,42,0),'SK_Infected_ArmRight':(0,-42,0),'SK_Infected_LegLeft':(0,32,-14)})]:
        for name in manifest['meshes']:bpy.data.objects[name].location=offsets.get(name,(0,0,0))
        camera.data.ortho_scale=300 if offsets else 270
        scene.render.filepath=str(SOURCE/('Blender_InfectedModular_'+label+'.png'))
        bpy.ops.render.render(write_still=True)
    for name in manifest['meshes']:bpy.data.objects[name].location=(0,0,0)
    rig.animation_data_create();rig.animation_data.action=bpy.data.actions[clip_name]
    rig.animation_data.action_slot=rig.animation_data.action.slots[0]
    scene.frame_set(49);camera.data.ortho_scale=290
    scene.render.filepath=str(SOURCE/'Blender_InfectedModular_AttackRight.png')
    bpy.ops.render.render(write_still=True)
    print('C03_SOURCE_PREVIEWS_WRITTEN not gameplay evidence')
