"""Original C05 locomotion/attacks on accepted rigs and surfaces.
Blender --background --python Scripts/create_candidate05_motion.py
Writes only C05 sources and animation-only FBX. Root serializes UE imports.
"""
from pathlib import Path
import json, math, hashlib, struct, sys
import bpy
from mathutils import Vector, Matrix, Quaternion
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/Characters/C05'
OUT=ROOT/'ArtSource/Exports/Candidate05'
SOURCE.mkdir(parents=True,exist_ok=True); OUT.mkdir(parents=True,exist_ok=True)
ATTACKS_ONLY='--attacks-only' in sys.argv
FPS=100
WALK_SPEED,WALK_DURATION=225.,.64
RUN_SPEED,RUN_DURATION=370.,.54
TURN_DURATION=.60
NAMES=('F','FR','R','BR','B','BL','L','FL')
scene=None
rig=None
def reset():
    for bone in rig.pose.bones:
        bone.location = (0, 0, 0)
        bone.rotation_mode = 'QUATERNION'
        bone.rotation_quaternion = (1, 0, 0, 0)
        bone.scale = (1, 1, 1)
    bpy.context.view_layer.update()


def move(name, offset):
    bone = rig.pose.bones[name]
    matrix = bone.matrix.copy()
    matrix.translation += Vector(offset)
    bone.matrix = matrix
    bpy.context.view_layer.update()


def orient_segment(name, head, tail):
    bone = rig.pose.bones[name]
    rest = rig.data.bones[name]
    rotation = (rest.tail_local-rest.head_local).normalized().rotation_difference((tail-head).normalized())
    matrix = rotation.to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4()
    matrix.translation = head
    bone.matrix = matrix
    bpy.context.view_layer.update()


def leg(side, goal, toe_yaw=0, pitch=0):
    upper, lower = 'thigh_'+side, 'calf_'+side
    a = rig.pose.bones[upper].head.copy()
    length1, length2 = rig.data.bones[upper].length, rig.data.bones[lower].length
    delta = goal-a
    distance = min(delta.length, length1+length2-.01)
    direction = delta.normalized()
    along = (length1*length1-length2*length2+distance*distance)/(2*distance)
    height = math.sqrt(max(0, length1*length1-along*along))
    pole = Vector((80, -9 if side == 'l' else 9, 53))-a
    bend = (pole-direction*pole.dot(direction)).normalized()
    knee = a+direction*along+bend*height
    orient_segment(upper, a, knee)
    orient_segment(lower, knee, goal)
    foot = rig.pose.bones['foot_'+side]
    rest = rig.data.bones['foot_'+side]
    rotation = Quaternion(Vector((0, 0, 1)), toe_yaw) @ Quaternion(Vector((0, 1, 0)), pitch)
    matrix = rotation.to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4()
    matrix.translation = goal
    foot.matrix = matrix
    bpy.context.view_layer.update()


def ensure_reach(targets):
    drop = 0.0
    for side, goal, _, _ in targets:
        hip = rig.pose.bones['thigh_'+side].head
        length = rig.data.bones['thigh_'+side].length+rig.data.bones['calf_'+side].length-.4
        horizontal = (goal.x-hip.x)**2+(goal.y-hip.y)**2
        drop = min(drop, goal.z+math.sqrt(max(0, length*length-horizontal))-hip.z)
    if drop < 0:
        move('pelvis', (0, 0, drop))


def smooth(value):
    value = max(0, min(1, value))
    return value*value*(3-2*value)


def gait(phase, yaw, speed, duration, running):
    # The source foot travels exactly speed*dt backwards during its stance.
    # Every direction is authored directly; no orthogonal stride shortening.
    direction = Vector((math.cos(yaw), -math.sin(yaw), 0))
    stance = .32 if running else .50
    excursion = speed*duration*stance
    # Shorter travel is authored geometrically at the same world speed. Transfer
    # toward the loaded leg replaces the previous constant 9–10cm crouch.
    transfer=math.sin(phase*math.tau)
    move('pelvis', (0, (1.25 if running else 1.65)*transfer,
        (-3.2 if running else -2.2)+(1.0 if running else .55)*math.cos(phase*math.tau*2)))
    rotate('pelvis',(0,0,1),math.radians(2.0)*transfer)
    rotate('spine_01',(0,1,0),math.radians(3.2 if running else 1.4))
    rotate('spine_01',(1,0,0),math.radians(-1.2)*transfer)
    rotate('spine_02',(0,0,1),math.radians(-2.0)*transfer)
    targets = []
    for side, sign, offset in (('l', -1, 0), ('r', 1, .5)):
        u = (phase+offset) % 1
        if u < stance:
            travel = excursion*(.5-u/stance)
            lift = 0
            pitch = .11*smooth((u/stance-.80)/.20)
        else:
            swing = (u-stance)/(1-stance)
            travel = excursion*(-.5+smooth(swing))
            lift = (16 if running else 9)*math.sin(math.pi*swing)**1.3
            pitch = -.10*math.sin(math.pi*swing)
        goal = Vector((0, sign*9, 10))+direction*travel+Vector((0, 0, lift))
        goal.z += max(0, 20*math.sin(pitch), -8*math.sin(pitch))+9.4*(math.cos(pitch)-1)
        # Raised crossover foot clears the supporting shoe during fast strafing.
        goal.x += abs(direction.y)*sign*5*math.sin(math.pi*max(0, (u-stance)/(1-stance)))**2 if u >= stance else 0
        toe_yaw = -math.sin(yaw)*math.radians(18 if running else 14)
        targets.append((side, goal, toe_yaw, pitch))
    ensure_reach(targets)
    for target in targets:
        leg(*target)


def turn_fraction(phase):
    # This is the body-yaw curve consumed by ONEPlayer::UpdateBodyFacing.
    return .5*smooth(phase*2) if phase < .5 else .5+.5*smooth((phase-.5)*2)


def turn(phase, ue_sign):
    yaw = -ue_sign*math.pi/2*turn_fraction(phase)
    inverse = Quaternion(Vector((0, 0, 1)), -yaw)
    # First step opens the turn; second brings the trailing foot into alignment.
    first = 'l' if ue_sign > 0 else 'r'
    move('pelvis', (0, .6*ue_sign*math.sin(phase*math.tau), -1.5*math.sin(math.pi*phase)**2))
    for side, sign in (('l', -1), ('r', 1)):
        begin = 0 if side == first else .5
        u = max(0, min(1, (phase-begin)/.5))
        foot_angle = -ue_sign*math.pi/2*smooth(u)
        base = Vector((0, sign*9, 10))
        goal = Quaternion(Vector((0, 0, 1)), foot_angle) @ base
        goal.z += 8*math.sin(math.pi*u)
        goal = inverse @ goal
        leg(side, goal, foot_angle-yaw, -.06*math.sin(math.pi*u))


def export_action(name, duration, pose):
    previous=bpy.data.actions.get(name)
    if previous: bpy.data.actions.remove(previous)
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = action
    frames = round(duration*FPS)
    scene.frame_start, scene.frame_end = 1, frames+1
    samples = []
    for index in range(frames+1):
        scene.frame_set(index+1)
        reset()
        pose(index/frames)
        for bone in rig.pose.bones:
            for channel in ('location', 'rotation_quaternion', 'scale'):
                bone.keyframe_insert(channel, frame=index+1, group=bone.name)
        samples.append({
            'time': index/FPS,
            'pelvis': list(rig.pose.bones['pelvis'].head),
            'hand_l': list(rig.pose.bones['hand_l'].head),
            'hand_r': list(rig.pose.bones['hand_r'].head),
            'knee_l': list(rig.pose.bones['calf_l'].head),
            'knee_r': list(rig.pose.bones['calf_r'].head),
            'foot_l': list(rig.pose.bones['foot_l'].head),
            'foot_r': list(rig.pose.bones['foot_r'].head),
            'ankle_gap_max': max((rig.pose.bones['calf_'+s].tail-rig.pose.bones['foot_'+s].head).length for s in ('l', 'r')),
        })
    scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_selection=True,
        object_types={'ARMATURE'}, axis_forward='-Y', axis_up='Z', global_scale=1,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', use_space_transform=True,
        bake_space_transform=False, add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
        use_armature_deform_only=False, bake_anim=True, bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False, bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0, path_mode='STRIP')
    return action, samples



def rotate(name,axis,angle):
    b=rig.pose.bones[name]; m=b.matrix.copy(); p=m.translation.copy()
    b.matrix=Matrix.Translation(p)@Quaternion(Vector(axis),angle).to_matrix().to_4x4()@Matrix.Translation(-p)@m
    bpy.context.view_layer.update()

def arm(side,goal,pole,hand_direction):
    upper,lower='upperarm_'+side,'lowerarm_'+side
    a=rig.pose.bones[upper].head.copy(); goal=Vector(goal)
    l1,l2=rig.data.bones[upper].length,rig.data.bones[lower].length
    delta=goal-a; distance=min(delta.length,l1+l2-.3); d=delta.normalized(); goal=a+d*distance
    along=(l1*l1-l2*l2+distance*distance)/(2*distance)
    h=math.sqrt(max(0,l1*l1-along*along)); p=Vector(pole)-a
    bend=(p-d*p.dot(d)).normalized(); elbow=a+d*along+bend*h
    orient_segment(upper,a,elbow); orient_segment(lower,elbow,goal)
    orient_segment('hand_'+side,goal,goal+Vector(hand_direction))

def curve(t,keys):
    if t<=keys[0][0]: return Vector(keys[0][1])
    for (a,x),(b,y) in zip(keys,keys[1:]):
        if t<=b:return Vector(x).lerp(Vector(y),smooth((t-a)/(b-a)))
    return Vector(keys[-1][1])

def attack(phase,family,anatomical):
    duration,contact,distance,stepend=((.96,.45,18,.34),(1.08,.48,12,.34),(1.12,.54,14,.38))[family]
    t=phase*duration; u=min(1,t/stepend)
    # Integral of the bounded runtime step-speed profile. No root bone movement.
    advance=distance*(u+u*u-u*u*u)
    source='r' if anatomical=='Left' else 'l'; sign=1 if source=='r' else -1
    if family==2: source,sign='r',1
    wind=smooth(t/.22); strike=smooth((t-.22)/(contact-.22)); recover=smooth((t-contact)/(duration-contact))
    load=math.sin(math.pi*phase)
    move('pelvis',(2.5*math.sin(math.pi*phase),sign*1.4*load,-2.5-2.2*math.sin(math.pi*phase)**2))
    torso_yaw=sign*((-15*wind+31*strike)*(1-recover)) if family!=2 else 0
    rotate('pelvis',(0,0,1),math.radians(torso_yaw*.22))
    rotate('spine_01',(0,1,0),math.radians(6+10*strike*(1-recover)))
    rotate('spine_01',(0,0,1),math.radians(torso_yaw*.5))
    rotate('spine_02',(0,0,1),math.radians(torso_yaw*.28))
    rotate('neck',(0,0,1),math.radians(-torso_yaw*.45))
    targets=[]
    for side,lateral in (('l',-1),('r',1)):
        lead=side==source
        if lead:
            foot_u=min(1,t/stepend); world_x=4+distance*smooth(foot_u)
            lift=8*math.sin(math.pi*foot_u)
        else:
            foot_u=max(0,min(1,(t-contact-.07)/(duration-contact-.07)))
            world_x=-4+distance*smooth(foot_u); lift=6*math.sin(math.pi*foot_u)
        targets.append((side,Vector((world_x-advance,lateral*10.5,10+lift)),math.radians(lateral*5),0))
    ensure_reach(targets)
    for item in targets:leg(*item)
    for side,lateral in (('l',-1),('r',1)):
        active=family==2 or side==source
        if not active:
            goal=(19+4*load,lateral*26,116+3*load)
        elif family==0:
            goal=curve(t,[(0,(19,lateral*27,116)),(.22,(8,lateral*31,137)),(contact,(57,lateral*2,126)),(contact+.17,(36,-lateral*22,108)),(duration,(19,lateral*27,116))])
        elif family==1:
            goal=curve(t,[(0,(19,lateral*27,116)),(.25,(20,lateral*5,135)),(contact,(55,lateral*11,132)),(contact+.22,(27,lateral*38,107)),(duration,(19,lateral*27,116))])
        else:
            goal=curve(t,[(0,(19,lateral*27,116)),(.28,(24,lateral*16,146)),(contact,(54,lateral*13,121)),(contact+.20,(34,lateral*18,103)),(duration,(19,lateral*27,116))])
        reach=math.sin(math.pi*min(1,t/contact)) if t<contact else 1-recover
        arm(side,goal,(15,lateral*36,119),(4+7*reach,-lateral*2*reach,-9+5*reach))

def infected_gait(phase,running):
    speed,duration=(195.,.66) if running else (100.,1.04)
    gait(phase,0,speed,duration,running)
    rotate('spine_01',(0,1,0),math.radians(7 if running else 9))
    rotate('spine_02',(0,0,1),math.radians(3)*math.sin(phase*math.tau))
    for side,sign,offset in (('l',-1,0),('r',1,.5)):
        wave=math.sin((phase+offset)*math.tau)
        arm(side,(19+6*wave,sign*(25+2*wave),113+4*wave),(0,sign*44,116),(5,0,-8))

def surface_digest():
    h=hashlib.sha256()
    for ob in sorted((o for o in bpy.data.objects if o.type=='MESH'),key=lambda o:o.name):
        h.update(ob.name.encode())
        for v in ob.data.vertices:
            h.update(struct.pack('<3f',*v.co))
            for g in v.groups:h.update(struct.pack('<If',g.group,g.weight))
        for poly in ob.data.polygons:
            h.update(struct.pack('<I',poly.material_index)); h.update(struct.pack('<'+'I'*len(poly.vertices),*poly.vertices))
    return h.hexdigest()

def open_source(path,who):
    global rig,scene
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/path)); scene=bpy.context.scene
    old_fps=scene.render.fps
    for a in bpy.data.actions:
        for layer in a.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    for fc in bag.fcurves:
                        for key in fc.keyframe_points:
                            for point in (key.co,key.handle_left,key.handle_right):point.x=1+(point.x-1)*FPS/old_fps
    scene.render.fps=FPS; scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
    rig=bpy.data.objects['Rig_'+who]; rig.animation_data_clear(); rig.location=(0,0,0)
    return surface_digest(),{b.name:[list(row) for row in b.matrix_local] for b in rig.data.bones}

inventory={'candidate':'05','fps':FPS,'destination':'/Game/ONE/Animations/Candidate05','clips':{},
    'source_axes':'+X forward; source Y reflects into Unreal Y; *_r is anatomical LEFT',
    'authorship':'Original analytical full skeletal poses on accepted editable 21-bone source rigs; no external animation or mesh assets.',
    'limitations':'Source invariants and previews do not establish runtime foot planting, hand contact or perceived naturalism.'}
checks={'result':'PASS','scope':'Source geometry/rig identity and sampled kinematic invariants only','sources':{},'clips':{}}
samples_all={}
def authored(who,key,duration,pose,extra):
    name='A_'+who+'_C05_'+key
    _,rows=export_action(name,duration,pose)
    definition={'duration':duration,'skeleton':'SK_'+who+'_Skeleton',**extra}
    inventory['clips'][name]=definition;samples_all[name]=rows
    gap=max(r['ankle_gap_max'] for r in rows)
    pelvis=[r['pelvis'][2] for r in rows]
    result={'samples':len(rows),'max_ankle_chain_gap_cm':gap,'pelvis_z_min_cm':min(pelvis),'pelvis_z_max_cm':max(pelvis)}
    assert gap<.002,(name,gap)
    if 'speed' in extra:
        endpoint=max((Vector(rows[0]['foot_'+s])-Vector(rows[-1]['foot_'+s])).length for s in ('l','r'))
        yaw=math.radians(extra['direction_degrees_ue']); vel=Vector((math.cos(yaw),-math.sin(yaw),0))*extra['speed'];err=0
        for side,offset in (('l',0),('r',.5)):
            for a,b in zip(rows,rows[1:]):
                ua=(a['time']/duration+offset)%1;ub=(b['time']/duration+offset)%1
                if .015<ua<ub<extra['stance_fraction']-.015:
                    drift=Vector(b['foot_'+side])-Vector(a['foot_'+side])+vel*(b['time']-a['time']);drift.z=0;err=max(err,drift.length)
        assert endpoint<.002 and err<.002,(name,endpoint,err)
        result.update(loop_endpoint_gap_cm=endpoint,source_support_drift_cm=err)
    if 'contact_time' in extra:
        contact=rows[round(extra['contact_time']*FPS)]
        result['contact_hands_source_cm']={s:contact['hand_'+s] for s in ('l','r')}
        active=['r'] if extra['required_arms']=='Left' else ['l'] if extra['required_arms']=='Right' else ['l','r']
        assert min(contact['hand_'+s][0] for s in active)>40,(name,'active arm did not reach')
    checks['clips'][name]=result

def save_source(who,before,rest,filename):
    assert before==surface_digest(),'Accepted mesh/weights/material assignments changed'
    assert rest=={b.name:[list(row) for row in b.matrix_local] for b in rig.data.bones},'Accepted bind pose changed'
    checks['sources'][who]={'mesh_weight_material_sha256':before,'mesh_weights_and_material_assignments_unchanged':True,'bind_matrices_unchanged':True,'bones':len(rest)}
    scene.render.filepath='//C05_preview.png';scene.render.use_stamp=False
    # Clear known UI-path memories. Publication sanitizer still checks final files.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='FILE_BROWSER' and area.spaces.active.params:area.spaces.active.params.directory=b'//'
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/filename))

if not ATTACKS_ONLY:
    before,rest=open_source('ArtSource/Characters/Candidate03/ResponseLocomotion.blend','Response')
    for gait_name,speed,duration,running in (('Walk',225.,.64,False),('Run',370.,.54,True)):
        for i,direction in enumerate(NAMES):
            authored('Response',gait_name+'_'+direction,duration,lambda p,y=i*math.pi/4,s=speed,d=duration,r=running:gait(p,y,s,d,r),
                {'speed':speed,'direction_degrees_ue':i*45,'stance_fraction':.32 if running else .5,'stride_cm':speed*duration})
    for direction,sign in (('L',-1),('R',1)):
        authored('Response','Turn_'+direction,.6,lambda p,s=sign:turn(p,s),{'turn_degrees_ue':sign*90,'body_yaw_fraction':'unchanged C03 two smooth steps'})
    rig.animation_data.action=bpy.data.actions['A_Response_C05_Walk_F'];scene.frame_set(1)
    save_source('Response',before,rest,'ResponseMotion.blend')
    before,rest=open_source('ArtSource/Characters/Candidate03/InfectedModular.blend','Infected')
    for key,speed,duration,running in (('Walk',100.,1.04,False),('Run',195.,.66,True)):
        authored('Infected',key,duration,lambda p,r=running:infected_gait(p,r),{'speed':speed,'direction_degrees_ue':0,'stance_fraction':.32 if running else .5,'stride_cm':speed*duration})
else:
    inventory=json.loads((SOURCE/'inventory.json').read_text())
    checks=json.loads((SOURCE/'source_validation.json').read_text())
    samples_all=json.loads((SOURCE/'source_samples.json').read_text())
    before,rest=open_source('ArtSource/Characters/C05/InfectedMotion.blend','Infected')
for family,key,duration,contact,step,end in ((0,'Swipe',.96,.45,18,.34),(1,'Rake',1.08,.48,12,.34),(2,'TwoHand',1.12,.54,14,.38)):
    for side in (('Both',) if family==2 else ('Left','Right')):
        authored('Infected',key+(side if family!=2 else ''),duration,lambda p,f=family,s=side:attack(p,f,s),
            {'attack_family':family,'required_arms':side,'contact_time':contact,'step_distance_cm':step,'step_end_time':end,'heading':'fixed at windup; one contact attempt'})
rig.animation_data.action=bpy.data.actions['A_Infected_C05_SwipeLeft'];scene.frame_set(46)
save_source('Infected',before,rest,'InfectedMotion.blend')
for name,d in inventory['clips'].items():d['fbx_sha256']=hashlib.sha256((OUT/(name+'.fbx')).read_bytes()).hexdigest()
(SOURCE/'inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
(SOURCE/'source_validation.json').write_text(json.dumps(checks,indent=2)+'\n')
(SOURCE/'source_samples.json').write_text(json.dumps(samples_all,separators=(',',':'))+'\n')
print('CANDIDATE05_MOTION_READY '+str(len(inventory['clips'])))
