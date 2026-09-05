"""Derive Candidate03 severable infected from Project ONE's accepted source.

Blender 5.1.2: blender --background --python Scripts/create_candidate03_infected.py
Reads Infected.blend; never regenerates or overwrites the accepted character.
Unreal imports and metadata sanitation are separately coordinated by root.
"""
from pathlib import Path
from collections import Counter
import bpy, bmesh, hashlib, json, math
from mathutils import Matrix, Vector

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/Characters/Candidate03'
EXPORT=ROOT/'ArtSource/Exports/Candidate03'
SOURCE.mkdir(parents=True,exist_ok=True); EXPORT.mkdir(parents=True,exist_ok=True)
ACCEPTED=ROOT/'ArtSource/Characters/Infected.blend'
FPS=100
SEAM='C03Seam'
CAP='C03Cap'
bpy.ops.wm.open_mainfile(filepath=str(ACCEPTED))
rig=bpy.data.objects['Rig_Infected']
originals={name:bpy.data.objects[name] for name in ('SK_Infected','SK_Infected_Head','SK_Infected_ArmL')}
scene=bpy.context.scene
for o in list(bpy.data.objects):
    if o not in [rig,*originals.values()]:bpy.data.objects.remove(o,do_unlink=True)
rig.animation_data_clear()
for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4)
scene.frame_set(1); bpy.context.view_layer.update()
REST={b.name:b.matrix_local.copy() for b in rig.data.bones}
assert len(REST)==21 and all(abs(v-1)<1e-7 for v in rig.scale)
BASE_CLIPS={name:bpy.data.actions[name] for name in ('A_Infected_Idle','A_Infected_Walk','A_Infected_Run','A_Infected_Attack','A_Infected_AttackOneArm','A_Infected_Hit','A_Infected_Death')}
for action in BASE_CLIPS.values():action.use_fake_user=True


def bind_action(action):
    rig.animation_data_create(); rig.animation_data.action=action
    if action and len(action.slots):rig.animation_data.action_slot=action.slots[0]


def set_frame(value):
    frame=math.floor(value);scene.frame_set(frame,subframe=value-frame)


def group_names(o,v):
    return {o.vertex_groups[g.group].name for g in v.groups if g.weight>1e-6}


def clone(o,name,predicate=None):
    result=o.copy(); result.data=o.data.copy(); result.name=name
    bpy.context.collection.objects.link(result)
    result.hide_render=False; result.hide_set(False)
    if predicate:
        bm=bmesh.new(); bm.from_mesh(result.data); bm.verts.ensure_lookup_table()
        remove=[bm.verts[v.index] for v in o.data.vertices if not predicate(v)]
        bmesh.ops.delete(bm,geom=remove,context='VERTS'); bm.to_mesh(result.data); bm.free()
    return result


def member(o,v,chain):
    names=group_names(o,v)
    return bool(names) and names.issubset(chain)


def faces_fingerprint(o,predicate=None):
    colors=o.data.color_attributes.get('Color')
    result=Counter()
    for f in o.data.polygons:
        if predicate and not predicate(f):continue
        corners=[]
        for li in f.loop_indices:
            v=o.data.vertices[o.data.loops[li].vertex_index]
            w=tuple(sorted((o.vertex_groups[g.group].name,round(g.weight,6)) for g in v.groups if g.weight>1e-7))
            color=tuple(round(x,6) for x in colors.data[li].color) if colors else ()
            corners.append((tuple(round(x,5) for x in v.co),w,color))
        result[(o.data.materials[f.material_index].name,tuple(sorted(corners)))]+=1
    return result


def ensure_material(o,name):
    for i,m in enumerate(o.data.materials):
        if m.name==name:return i
    o.data.materials.append(bpy.data.materials[name]);return len(o.data.materials)-1


def cap_loops(o,point,direction,seam_id,bone,distal):
    """Close every new cut loop; matching rim vertices retain original weights."""
    point=Vector(point); direction=Vector(direction).normalized()
    outward=-direction if distal else direction
    gore=ensure_material(o,'M_Gore'); exposed=ensure_material(o,'M_ExposedBone')
    cloth=ensure_material(o,'M_OrangeFabric')
    bm=bmesh.new();bm.from_mesh(o.data)
    marks=bm.verts.layers.int.get(SEAM) or bm.verts.layers.int.new(SEAM)
    caps=bm.faces.layers.int.get(CAP) or bm.faces.layers.int.new(CAP)
    deform=bm.verts.layers.deform.verify()
    color=bm.loops.layers.float_color.get('Color') or bm.loops.layers.float_color.new('Color')
    group=o.vertex_groups.get(bone) or o.vertex_groups.new(name=bone)
    edges={e for e in bm.edges if e.is_boundary and all(abs((v.co-point).dot(direction))<.0002 for v in e.verts)}
    loops=[]
    while edges:
        edge=next(iter(edges)); start=edge.verts[0]; current=start; previous=None; ring=[]
        while True:
            ring.append(current)
            choices=[e for e in current.link_edges if e in edges]
            if not choices:raise AssertionError('Unclosed cut loop '+o.name)
            chosen=choices[0];edges.remove(chosen);following=chosen.other_vert(current)
            if following==start:break
            previous,current=current,following
            if len(ring)>2048:raise AssertionError('Invalid cut traversal')
        loops.append(ring)
    assert loops,'No cut boundary '+o.name
    # Largest boundary is tissue; intersecting pocket boundaries close as cloth.
    loops.sort(key=lambda ring:max((v.co-sum((x.co for x in ring),Vector())/len(ring)).length for v in ring),reverse=True)
    summary=[]
    for loop_index,ring in enumerate(loops):
        center=sum((v.co for v in ring),Vector())/len(ring)
        normal=sum((ring[i].co.cross(ring[(i+1)%len(ring)].co) for i in range(len(ring))),Vector())
        if normal.dot(outward)<0:ring.reverse()
        for v in ring:v[marks]=seam_id
        # Sleeve/thigh cuts are above the joint blend. Keep the exact rim weights.
        for v in ring:
            weights=dict(v[deform]);assert abs(sum(weights.values())-1)<.0001
            assert abs(weights.get(group.index,0)-1)<.0001,'Cut must stay in single-bone proximal region'
        new_faces=[]
        if loop_index:
            f=bm.faces.new(ring);f.material_index=cloth;new_faces.append(f)
        else:
            inner=[];middle=[];bone_ring=[]
            for i,v in enumerate(ring):
                delta=v.co-center
                q=bm.verts.new(center+delta*.84-outward*.22)
                q[deform][group.index]=1;inner.append(q)
                q=bm.verts.new(center+delta*(.48+.035*math.sin(i*2.3))-outward*(.30+.065*math.sin(i*1.9)))
                q[deform][group.index]=1;middle.append(q)
                radius=(1.45 if bone.startswith('thigh') else 1.15)*(1+.055*math.sin(i*2.7))
                q=bm.verts.new(center+delta.normalized()*radius-outward*.34)
                q[deform][group.index]=1;bone_ring.append(q)
            for outer,inside in ((ring,inner),(inner,middle),(middle,bone_ring)):
                for i in range(len(ring)):
                    j=(i+1)%len(ring);f=bm.faces.new((outer[i],outer[j],inside[j],inside[i]))
                    f.material_index=gore;new_faces.append(f)
            f=bm.faces.new(bone_ring);f.material_index=exposed;new_faces.append(f)
        for f in new_faces:
            f[caps]=seam_id;f.smooth=False
            for j,loop in enumerate(f.loops):
                position=loop.vert.co
                grain=math.sin(position.x*2.3+position.y*1.7)*math.sin(position.z*2.1+position.x*.9)
                shade=.76+.16*grain
                loop[color]=(shade,shade*.78,shade*.72,1) if f.material_index==gore else (.86,.81,.71,1)
        assert all(not e.is_boundary for v in ring for e in v.link_edges),'Open seam after capping'
        summary.append({'rim_vertices':len(ring),'center_source_cm':list(center),'max_radius_cm':max((v.co-center).length for v in ring),'surface':'tissue' if loop_index==0 else 'intersected pocket cloth'})
    bm.to_mesh(o.data);bm.free();o.data.update()
    return summary


def cut(o,point,normal,seam_id,bone,distal):
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.00001,
        plane_co=Vector(point),plane_no=Vector(normal),clear_inner=distal,clear_outer=not distal)
    bm.to_mesh(o.data);bm.free();o.data.update()
    return cap_loops(o,point,normal,seam_id,bone,distal)


def join(objects,name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.hide_set(False);o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join()
    result=bpy.context.object;result.name=name
    return result


body=originals['SK_Infected']; old_right=originals['SK_Infected_ArmL']
arm_chain={'upperarm_r','lowerarm_r','hand_r'}
leg_chain={'thigh_r','calf_r','foot_r','toe_r'}
left_arm=clone(body,'SK_Infected_ArmLeft',lambda v:member(body,v,arm_chain))
left_leg=clone(body,'SK_Infected_LegLeft',lambda v:member(body,v,leg_chain))
core=clone(body,'SK_Infected_Core',lambda v:not(member(body,v,arm_chain) or member(body,v,leg_chain) or member(body,v,{'upperarm_l'})))
arm_upper=clone(left_arm,'New_left_shoulder_stub')
leg_upper=clone(left_leg,'New_left_thigh_stub')
head=clone(originals['SK_Infected_Head'],'SK_Infected_Head_C03')
head.name='SK_Infected_Head'
right_arm=clone(old_right,'SK_Infected_ArmRight')

def bone_cut(name,distance=None,fraction=None):
    b=rig.data.bones[name];n=(b.tail_local-b.head_local).normalized()
    p=b.head_local+n*distance if distance is not None else b.head_local.lerp(b.tail_local,fraction)
    return p,n

left_point,left_normal=bone_cut('upperarm_r',distance=4.8)
right_point,right_normal=bone_cut('upperarm_l',distance=4.8)
leg_point,leg_normal=bone_cut('thigh_r',fraction=.38)
CUTS={
 'ArmLeft':{'id':1,'bone':'upperarm_r','point':left_point,'normal':left_normal,'part':left_arm},
 'ArmRight':{'id':2,'bone':'upperarm_l','point':right_point,'normal':right_normal,'part':right_arm},
 'LegLeft':{'id':3,'bone':'thigh_r','point':leg_point,'normal':leg_normal,'part':left_leg},
}
for definition,upper in ((CUTS['ArmLeft'],arm_upper),(CUTS['LegLeft'],leg_upper)):
    for o,distal in ((definition['part'],True),(upper,False)):
        loops=cut(o,definition['point'],definition['normal'],definition['id'],definition['bone'],distal)
        definition['distal_loops' if distal else 'proximal_loops']=loops

# Reuse the accepted right arm's exterior. Replace only its old interior cap and
# short core stump so both sides use the same 28-vertex rim, not 28 versus 32.
bm=bmesh.new();bm.from_mesh(right_arm.data)
removed=[]
for f in bm.faces:
    material=right_arm.data.materials[f.material_index].name
    if material in {'M_Gore','M_ExposedBone'} or all(abs((v.co-right_point).dot(right_normal))<.0002 for v in f.verts):removed.append(f)
bmesh.ops.delete(bm,geom=removed,context='FACES')
bmesh.ops.delete(bm,geom=[v for v in bm.verts if not v.link_faces],context='VERTS')
bm.to_mesh(right_arm.data);bm.free();right_arm.data.update()
CUTS['ArmRight']['distal_loops']=cap_loops(right_arm,right_point,right_normal,2,'upperarm_l',True)
rim_attr=right_arm.data.attributes[SEAM]
rim=[v.co.copy() for v in right_arm.data.vertices if rim_attr.data[v.index].value==2]
assert len(rim)==28
# Sort angularly in the exact accepted cross-sectional plane.
axis=(rim[0]-right_point).normalized();side=right_normal.cross(axis).normalized()
rim.sort(key=lambda v:math.atan2((v-right_point).dot(side),(v-right_point).dot(axis)))
vertices=[]
for fraction,radial in ((0,1.015),(.30,1.05),(1,1)):
    center=rig.data.bones['upperarm_l'].head_local.lerp(right_point,fraction)
    vertices.extend(center+(v-right_point)*radial for v in rim)
faces=[]
for row in range(2):
    for i in range(28):j=(i+1)%28;faces.append((row*28+i,row*28+j,(row+1)*28+j,(row+1)*28+i))
faces.append(tuple(reversed(range(28))))
mesh=bpy.data.meshes.new('Matched_right_shoulder_stub');mesh.from_pydata(vertices,[],faces);mesh.update()
right_stub=bpy.data.objects.new('Matched_right_shoulder_stub',mesh);bpy.context.collection.objects.link(right_stub)
mesh.materials.append(bpy.data.materials['M_OrangeFabric'])
vg=right_stub.vertex_groups.new(name='upperarm_l');vg.add(list(range(len(vertices))),1,'REPLACE')
paint=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
for loop in paint.data:loop.color=(.89,.89,.89,1)
for f in mesh.polygons:f.use_smooth=len(f.vertices)==4
right_stub.parent=rig;mod=right_stub.modifiers.new('Reusable weighted skeleton','ARMATURE');mod.object=rig
CUTS['ArmRight']['proximal_loops']=cap_loops(right_stub,right_point,right_normal,2,'upperarm_l',False)
core=join([core,arm_upper,leg_upper,right_stub],'SK_Infected_Core')
meshes=[core,head,left_arm,right_arm,left_leg]

# Source references remain editable and hidden in their own collection. Avoid
# colliding object names in FBX exports while retaining all accepted actions.
reference=bpy.data.collections.new('Accepted source reference — hidden');scene.collection.children.link(reference)
for name,o in originals.items():
    o.name=name+'_AcceptedReference'
    for collection in list(o.users_collection):collection.objects.unlink(o)
    reference.objects.link(o);o.hide_render=True;o.hide_set(True)
head.name='SK_Infected_Head'

# Mirror the actual evaluated one-arm action across source Y using bind-space
# deformation matrices. This accounts for asymmetric bone bases; negating local
# Euler channels or renaming bones alone would not produce the correct motion.
mirror=Matrix.Diagonal((1,-1,1,1))
samples=[]
for frame in range(FPS+1):
    bind_action(BASE_CLIPS['A_Infected_AttackOneArm']);set_frame(1+frame/FPS*30)
    bpy.context.view_layer.update();pose={b.name:b.matrix.copy() for b in rig.pose.bones}
    target={}
    for name in REST:
        other=name[:-2]+('_r' if name.endswith('_l') else '_l') if name.endswith(('_l','_r')) and name!='weapon_r' else name
        if other not in REST:other=name
        target[name]=mirror@pose[other]@REST[other].inverted()@mirror@REST[name]
    samples.append(target)
action=bpy.data.actions.new('A_Infected_C03_AttackRight');action.use_fake_user=True
action['authored_fps']=FPS;action['duration_seconds']=1.0;action['contact_seconds']=.48
action['anatomical_strike_side']='Unreal anatomical right uses source hand_l'
bind_action(action)
scene.render.fps=FPS
for index,target in enumerate(samples):
    scene.frame_set(index+1)
    for b in rig.pose.bones:
        parent=b.parent.name if b.parent else None
        local=REST[parent].inverted()@REST[b.name] if parent else REST[b.name]
        b.rotation_mode='QUATERNION'
        b.matrix_basis=local.inverted()@(target[parent].inverted() if parent else Matrix.Identity(4))@target[b.name]
        for channel in ('location','rotation_quaternion','scale'):b.keyframe_insert(channel,frame=index+1,group=b.name)
bpy.context.view_layer.update()


def evaluate_seams():
    report=[]
    durations={'Idle':3,'Walk':1.4,'Run':.8,'Attack':1,'AttackOneArm':1,'Hit':.4,'Death':1.2,'C03_AttackRight':1}
    core_marks=core.data.attributes[SEAM]
    for suffix,duration in durations.items():
        clip=action if suffix=='C03_AttackRight' else BASE_CLIPS['A_Infected_'+suffix]
        bind_action(clip);fps=FPS if clip==action else 30
        for normalized in (0,.12,.25,.48,.63,.85,1):
            set_frame(1+duration*normalized*fps);bpy.context.view_layer.update()
            deps=bpy.context.evaluated_depsgraph_get();a=core.evaluated_get(deps).data
            for label,definition in CUTS.items():
                part=definition['part'];marks=part.data.attributes[SEAM];b=part.evaluated_get(deps).data
                ca=[a.vertices[i].co for i,x in enumerate(core_marks.data) if x.value==definition['id']]
                cb=[b.vertices[i].co for i,x in enumerate(marks.data) if x.value==definition['id']]
                assert len(ca)==len(cb),(label,len(ca),len(cb))
                error=max(max(min((x-y).length for y in cb) for x in ca),max(min((x-y).length for y in ca) for x in cb))
                assert error<.001,(suffix,normalized,label,error)
                report.append({'clip':clip.name,'normalized_time':normalized,'region':label,'rim_vertex_count':len(ca),'max_gap_cm':error})
    return report


validation={'result':'PASS','accepted_source_sha256':hashlib.sha256(ACCEPTED.read_bytes()).hexdigest(),
    'rest_matrices_unchanged':all(max(abs(REST[b.name][i][j]-b.matrix_local[i][j]) for i in range(4) for j in range(4))<1e-8 for b in rig.data.bones),
    'animated_seams':evaluate_seams(),'meshes':{}}
accepted=Counter()
for o in originals.values():accepted.update(faces_fingerprint(o))
derived=Counter()
for o in meshes:derived.update(faces_fingerprint(o))
unchanged=sum((accepted&derived).values())
validation['surface_preservation']={'accepted_polygons':sum(accepted.values()),'exactly_retained_polygons':unchanged,
    'changed_or_bisected_polygons':sum(accepted.values())-unchanged,
    'scope':'Only sever-plane intersections, existing right-arm interior cap and short proximal shoulder stump may differ; all head surfaces are copied unchanged.',
    'head_faces_identical':faces_fingerprint(head)==faces_fingerprint(originals['SK_Infected_Head'])}
def allowed_body_change(face):
    verts=[body.data.vertices[i] for i in face.vertices]
    groups=set().union(*(group_names(body,v) for v in verts))
    if groups=={'upperarm_l'}:return True
    for chain,p,n in ((arm_chain,left_point,left_normal),(leg_chain,leg_point,leg_normal)):
        distances=[(v.co-p).dot(n) for v in verts]
        if groups.issubset(chain) and min(distances)<=.0002 and max(distances)>=-.0002:return True
    return False
def allowed_right_cap_change(face):
    return old_right.data.materials[face.material_index].name in {'M_Gore','M_ExposedBone'} or all(abs((old_right.data.vertices[i].co-right_point).dot(right_normal))<.0002 for i in face.vertices)
allowed=faces_fingerprint(body,allowed_body_change)+faces_fingerprint(old_right,allowed_right_cap_change)
unauthorized=(accepted-derived)-allowed
validation['surface_preservation']['unauthorized_changed_polygons']=sum(unauthorized.values())
assert not unauthorized,'An unaffected accepted surface changed'
assert validation['rest_matrices_unchanged'] and validation['surface_preservation']['head_faces_identical']
assert unchanged/sum(accepted.values())>.975,'Too much accepted geometry changed'
for o in meshes:
    o.data.calc_loop_triangles()
    totals=[sum(g.weight for g in v.groups) for v in o.data.vertices]
    assert totals and min(totals)>.9999 and max(totals)<1.0001
    assert all(math.isfinite(x) for v in o.data.vertices for x in v.co)
    validation['meshes'][o.name]={'vertices':len(o.data.vertices),'triangles':len(o.data.loop_triangles),
        'weight_sum_min':min(totals),'weight_sum_max':max(totals),'materials':[m.name for m in o.data.materials]}
bind_action(action);scene.frame_set(49);bpy.context.view_layer.update()
for name in ('hand_l','hand_r'):
    validation[name+'_contact_head_source_cm']=list(rig.pose.bones[name].head)
assert rig.pose.bones['hand_l'].head.x>45,'Complementary anatomical right arm did not extend'
assert rig.pose.bones['hand_l'].head.x-rig.pose.bones['hand_r'].head.x>25,'Attack must favor the present right arm'


def export(path,objects,animated=False):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.hide_set(False);o.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'},
        axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
        use_space_transform=True,bake_space_transform=False,add_leaf_bones=False,
        primary_bone_axis='Y',secondary_bone_axis='X',use_armature_deform_only=False,
        mesh_smooth_type='FACE',use_mesh_modifiers=True,bake_anim=animated,bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0,path_mode='AUTO')


bind_action(None)
for b in rig.pose.bones:b.matrix_basis=Matrix.Identity(4)
scene.frame_set(1);bpy.context.view_layer.update()
for o in meshes:export(EXPORT/(o.name+'.fbx'),[rig,o])
bind_action(action);scene.frame_start=1;scene.frame_end=101
export(EXPORT/(action.name+'.fbx'),[rig],True)
inventory={'candidate':'03','accepted_source':'ArtSource/Characters/Infected.blend',
    'editable_source':'ArtSource/Characters/Candidate03/InfectedModular.blend','fps':FPS,
    'skeleton_asset':'/Game/ONE/Characters/SK_Infected_Skeleton',
    'coordinate_contract':'Centimetres, source +X forward/+Z up; UE preserves X and reflects source Y. Source _r is anatomical left.',
    'meshes':{o.name:{**validation['meshes'][o.name],'source':'ArtSource/Exports/Candidate03/'+o.name+'.fbx','asset':'/Game/ONE/Characters/Candidate03/'+o.name} for o in meshes},
    'clips':{action.name:{'duration':1,'contact_seconds':.48,'source':'ArtSource/Exports/Candidate03/'+action.name+'.fbx','asset':'/Game/ONE/Animations/Candidate03/'+action.name,'present_anatomical_arm':'right','strike_bone':'hand_l'}},
    'cuts':{},'retained_actions':list(BASE_CLIPS),'retained_action_source_fps':30}
for label,d in CUTS.items():
    p=d['point'];n=d['normal'];local=REST[d['bone']].inverted()@p
    inventory['cuts'][label]={'part':d['part'].name,'bone':d['bone'],'seam_id':d['id'],
        'source_component_cm':list(p),'ue_component_cm':[p.x,-p.y,p.z],
        'source_bone_local_cm':list(local),'source_normal':list(n),'ue_component_normal':[n.x,-n.y,n.z],
        'bone_bind_source_matrix':[list(row) for row in REST[d['bone']]],
        'distal_loops':d['distal_loops'],'proximal_loops':d['proximal_loops'],
        'runtime_stump_contract':'Retain cut-root bone evaluation and weights. Remove its physics body chain, never hide/scale the bone.'}
inventory['cuts']['Head']={'part':head.name,'bone':'head','source_component_cm':[0,0,157.8],'ue_component_cm':[0,0,157.8],
    'source_bone_local_cm':list(REST['head'].inverted()@Vector((0,0,157.8))),
    'contract':'Accepted closed head and neck caps retained without geometry changes.'}
(SOURCE/'infected_inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
(SOURCE/'infected_validation.json').write_text(json.dumps(validation,indent=2)+'\n')

# Portable editable file: no inherited remembered absolute file-browser or render
# paths. Root's metadata sanitizer still checks final FBX and blend binaries.
for screen in bpy.data.screens:
    for area in screen.areas:
        for space in area.spaces:
            if space.type=='FILE_BROWSER' and space.params:
                space.params.directory=b'//';space.params.filename=''
scene.render.filepath='//InfectedModularPreview.png'
scene.frame_set(1);bind_action(BASE_CLIPS['A_Infected_Idle'])
scene.render.fps=30;scene.frame_end=91
scene.world.color=(.06,.06,.06)
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'InfectedModular.blend'),compress=True)
print('C03_INFECTED_SOURCE_PASS',json.dumps({'meshes':len(meshes),'seam_samples':len(validation['animated_seams']),
    'max_seam_gap_cm':max(x['max_gap_cm'] for x in validation['animated_seams']),'preserved_polygons':unchanged,'accepted_polygons':sum(accepted.values())}))
