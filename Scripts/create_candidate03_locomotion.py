"""Author Candidate03 locomotion on the unchanged Response rig.

Run Blender --background --python Scripts/create_candidate03_locomotion.py.
Only Candidate03 sources/exports are written. UE directions name the clips;
source Y is reflected deliberately to match the established FBX import.
"""
from pathlib import Path
import json
import math
import bpy
from mathutils import Vector, Matrix, Quaternion

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'ArtSource/Characters/Candidate03'
OUT = ROOT/'ArtSource/Exports/Candidate03'
SOURCE.mkdir(parents=True, exist_ok=True)
OUT.mkdir(parents=True, exist_ok=True)
FPS = 100
WALK_SPEED, WALK_DURATION = 225.0, .72
RUN_SPEED, RUN_DURATION = 370.0, .62
TURN_DURATION = .60
NAMES = ('F', 'FR', 'R', 'BR', 'B', 'BL', 'L', 'FL')
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'ArtSource/Characters/Response.blend'))
scene = bpy.context.scene
old_fps = scene.render.fps
for action in bpy.data.actions:
    for layer in action.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for curve in bag.fcurves:
                    for key in curve.keyframe_points:
                        for coordinate in (key.co, key.handle_left, key.handle_right):
                            coordinate.x = 1+(coordinate.x-1)*FPS/old_fps
scene.render.fps = FPS
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01
rig = bpy.data.objects['Rig_Response']
rig.location = (0, 0, 0)
rig.animation_data_clear()


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
    stance = .35 if running else .51
    excursion = speed*duration*stance
    move('pelvis', (0, 0, (-10 if running else -9)+(2 if running else 1)*math.cos(phase*math.tau*2)))
    targets = []
    for side, sign, offset in (('l', -1, 0), ('r', 1, .5)):
        u = (phase+offset) % 1
        if u < stance:
            travel = excursion*(.5-u/stance)
            lift = 0
            pitch = .10*smooth((u/stance-.82)/.18)
        else:
            swing = (u-stance)/(1-stance)
            travel = excursion*(-.5+smooth(swing))
            lift = (24 if running else 15)*math.sin(math.pi*swing)
            pitch = -.13*math.sin(math.pi*swing)
        goal = Vector((0, sign*9, 10))+direction*travel+Vector((0, 0, lift))
        goal.z += max(0, 20*math.sin(pitch), -8*math.sin(pitch))+9.4*(math.cos(pitch)-1)
        # Raised crossover foot clears the supporting shoe during fast strafing.
        goal.x += abs(direction.y)*sign*5*math.sin(math.pi*max(0, (u-stance)/(1-stance)))**2 if u >= stance else 0
        toe_yaw = -math.sin(yaw)*math.radians(25 if running else 18)
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
    move('pelvis', (0, 0, -4*math.sin(math.pi*phase)**2))
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


inventory = {'source': 'ResponseLocomotion.blend', 'fps': FPS, 'skeleton': 'SK_Response_Skeleton',
    'source_axes': '+X forward, source Y reflected to UE Y', 'directions_are': 'Unreal actor-local +X forward / +Y right',
    'walk_speed_cm_s': WALK_SPEED, 'run_speed_cm_s': RUN_SPEED,
    'turn_duration': TURN_DURATION, 'turn_degrees': 90, 'clips': {},
    'limitations': 'Source numeric checks do not establish runtime foot planting or perceived motion quality.'}
validation = {}
for gait_name, speed, duration in (('Walk', WALK_SPEED, WALK_DURATION), ('Run', RUN_SPEED, RUN_DURATION)):
    for index, direction in enumerate(NAMES):
        name = 'A_Response_C03_'+gait_name+'_'+direction
        yaw = index*math.pi/4
        action, samples = export_action(name, duration, lambda phase, y=yaw, s=speed, d=duration, r=gait_name=='Run': gait(phase, y, s, d, r))
        inventory['clips'][name] = {'duration': duration, 'speed': speed, 'stride_cm': speed*duration,
            'direction_degrees_ue': index*45, 'stance_fraction': .35 if gait_name=='Run' else .51}
        validation[name] = samples
for direction, sign in (('L', -1), ('R', 1)):
    name = 'A_Response_C03_Turn_'+direction
    action, samples = export_action(name, TURN_DURATION, lambda phase, s=sign: turn(phase, s))
    inventory['clips'][name] = {'duration': TURN_DURATION, 'turn_degrees_ue': 90*sign,
        'body_yaw_fraction': '0.5*smoothstep(2*t) for t<0.5; 0.5+0.5*smoothstep(2*t-1) otherwise'}
    validation[name] = samples
    inventory['clips'][name]['ankle_gap_max_cm'] = max(s['ankle_gap_max'] for s in samples)
rig.animation_data.action = bpy.data.actions['A_Response_C03_Walk_F']
scene.frame_start, scene.frame_end = 1, round(WALK_DURATION*FPS)+1
scene.frame_set(1)
scene.render.filepath = '//Blender_LocomotionPreview.png'
# Never write over the accepted source; new actions and old editable references coexist.
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'ResponseLocomotion.blend'))
(SOURCE/'inventory.json').write_text(json.dumps(inventory, indent=2)+'\n', encoding='utf-8')
(SOURCE/'source_samples.json').write_text(json.dumps(validation, separators=(',', ':'))+'\n', encoding='utf-8')
checks = {'result': 'PASS', 'scope': 'Numeric source checks, not runtime or perceptual approval', 'clips': {}}
for name, samples in validation.items():
    definition = inventory['clips'][name]
    maximum_gap = max(row['ankle_gap_max'] for row in samples)
    loop_gap = None
    support_error = 0.0
    if 'speed' in definition:
        loop_gap = max((Vector(samples[0]['foot_'+side])-Vector(samples[-1]['foot_'+side])).length for side in ('l', 'r'))
        theta = math.radians(definition['direction_degrees_ue'])
        velocity = Vector((math.cos(theta), -math.sin(theta), 0))*definition['speed']
        for side, offset in (('l', 0), ('r', .5)):
            for a, b in zip(samples, samples[1:]):
                ua = (a['time']/definition['duration']+offset) % 1
                ub = (b['time']/definition['duration']+offset) % 1
                if .015 < ua < ub < definition['stance_fraction']-.015:
                    error = Vector(b['foot_'+side])-Vector(a['foot_'+side])+velocity*(b['time']-a['time'])
                    error.z = 0
                    support_error = max(support_error, error.length)
    else:
        sign = definition['turn_degrees_ue']/90
        first = 'l' if sign > 0 else 'r'
        for row in samples:
            phase = row['time']/definition['duration']
            body_yaw = -sign*math.pi/2*turn_fraction(phase)
            for side, lateral in (('l', -9), ('r', 9)):
                planted = (side != first and phase <= .5) or (side == first and phase >= .5)
                if planted:
                    expected = Vector((0, lateral, 10))
                    if side == first:
                        expected = Quaternion(Vector((0, 0, 1)), -sign*math.pi/2) @ expected
                    actual = Quaternion(Vector((0, 0, 1)), body_yaw) @ Vector(row['foot_'+side])
                    support_error = max(support_error, (actual-expected).length)
    checks['clips'][name] = {'max_ankle_chain_gap_cm': maximum_gap, 'loop_endpoint_gap_cm': loop_gap,
        'source_support_drift_cm': support_error, 'samples': len(samples)}
    if maximum_gap > .001 or (loop_gap is not None and loop_gap > .001) or support_error > .001:
        checks['result'] = 'FAIL'
(SOURCE/'source_validation.json').write_text(json.dumps(checks, indent=2)+'\n', encoding='utf-8')
assert checks['result'] == 'PASS', 'Source leg/contact invariant failed'
print('CANDIDATE03_LOCOMOTION_READY '+json.dumps({'clips': len(inventory['clips']), 'max_ankle_gap': max(v['ankle_gap_max'] for samples in validation.values() for v in samples)}))
