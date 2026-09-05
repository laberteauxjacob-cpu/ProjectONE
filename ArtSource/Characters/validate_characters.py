"""Read-only source/FBX QA; generates local Blender QA views and JSON measurements."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector
BASE=Path(__file__).resolve().parent
EXPORTS=BASE.parent/'Exports'
report={}
bpy.ops.wm.open_mainfile(filepath=str(BASE/'CharacterWorkshop.blend'))
scene=bpy.context.scene
for kind in ['Response','Infected']:
    rig=bpy.data.objects['Rig_'+kind]
    report[kind]={'actions':{},'weights':{}}
    for mesh in [o for o in bpy.data.objects if o.type=='MESH' and o.name.startswith('SK_'+kind)]:
        sums=[sum(g.weight for g in v.groups) for v in mesh.data.vertices]
        report[kind]['weights'][mesh.name]={'min':min(sums),'max':max(sums),'unweighted':sum(v<.999 for v in sums)}
    for action in [a for a in bpy.data.actions if a.name.startswith('A_'+kind+'_')]:
        rig.animation_data.action=action
        samples=[]
        for t in [0,.25,.5,.75,1]:
            scene.frame_set(round(action.frame_range[0]+t*(action.frame_range[1]-action.frame_range[0])))
            bpy.context.view_layer.update()
            coords=[]
            for mesh in [o for o in bpy.data.objects if o.type=='MESH' and o.name.startswith('SK_'+kind)]:
                evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
                coords.extend([evaluated.matrix_world@v.co for v in evaluated.data.vertices])
            samples.append({'phase':t,'pelvis':list(rig.pose.bones['pelvis'].head),
                'head':list(rig.pose.bones['head'].head),
                'left_foot':list(rig.pose.bones['foot_l'].head),
                'right_foot':list(rig.pose.bones['foot_r'].head),
                'left_ankle_gap':(rig.pose.bones['calf_l'].tail-rig.pose.bones['foot_l'].head).length,
                'right_ankle_gap':(rig.pose.bones['calf_r'].tail-rig.pose.bones['foot_r'].head).length,
                'mesh_z_min':min(v.z for v in coords),'mesh_z_max':max(v.z for v in coords)})
        report[kind]['actions'][action.name]={'range':list(action.frame_range),'samples':samples}
    rig.animation_data.action=bpy.data.actions['A_'+kind+'_Idle']
scene.frame_set(1)
# Readable top-down source QA (not gameplay evidence).
scene.camera.location=(280,-260,430)
scene.camera.rotation_euler=(Vector((0,0,80))-scene.camera.location).to_track_quat('-Z','Y').to_euler()
scene.camera.data.ortho_scale=320
scene.render.resolution_x=1200;scene.render.resolution_y=900
scene.render.filepath=str(BASE/'Blender_GaitQuarter.png')
bpy.data.objects['Rig_Response'].animation_data.action=bpy.data.actions['A_Response_Run']
bpy.data.objects['Rig_Infected'].animation_data.action=bpy.data.actions['A_Infected_Run']
scene.frame_set(7); bpy.ops.render.render(write_still=True)
scene.render.filepath=str(BASE/'Blender_DeathAndReload.png')
bpy.data.objects['Rig_Response'].animation_data.action=bpy.data.actions['A_Response_Reload']
bpy.data.objects['Rig_Infected'].animation_data.action=bpy.data.actions['A_Infected_Death']
scene.frame_set(32); bpy.ops.render.render(write_still=True)
for kind in ['Response','Infected']:
    for obj in list(bpy.data.objects): bpy.data.objects.remove(obj,do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=str(EXPORTS/('SK_'+kind+'.fbx')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    report[kind]['fbx_roundtrip']={'bounds':list(mesh.dimensions),'rig_scale':list(rig.scale),'root_head':list(rig.data.bones['root'].head_local),'weapon_matrix':[list(row) for row in rig.data.bones['weapon_r'].matrix_local], 'color_attributes':list(mesh.data.color_attributes.keys())}
(BASE/'validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps({kind:{'weights':v['weights'],'fbx_roundtrip':v['fbx_roundtrip']} for kind,v in report.items()},indent=2))
