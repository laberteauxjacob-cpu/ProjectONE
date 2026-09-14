"""Author only C07 infected motion on the redesigned Maintenance character.

Blender --background --python Scripts/create_candidate07_infected_motion.py
The operator serializes Blender/UE work. No player file, existing animation,
bind matrix, mesh, weight or material assignment is modified. The compatible
character source must exist first; there is no old-mannequin fallback.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
import sys
import bpy
from mathutils import Matrix, Quaternion, Vector

ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / 'ArtSource/Characters/Candidate07/Maintenance.blend'
SOURCE = ROOT / 'ArtSource/Characters/Candidate07/Motion'
EXPORT = ROOT / 'ArtSource/Exports/Candidate07/Motion'
FPS = 100
parser = argparse.ArgumentParser()
parser.add_argument('--recoveries-only', action='store_true', help='Refresh current character source and only the two recovery FBXs; retain the other twelve actions and files exactly.')
parser.add_argument('--retained-source', type=Path, help='Explicit preserved Motion.blend supplying the twelve original actions when recovering an incomplete refresh.')
parser.add_argument('--render-recovery', action='store_true', help='Render bounded chronological source contact sheets for the two recoveries.')
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
RECOVERIES = {'A_Infected_C07_GetUpProne', 'A_Infected_C07_GetUpSupine'}
GUARD_PATHS = [INPUT, ROOT/'ArtSource/Characters/Response.blend', ROOT/'ArtSource/Characters/C05/ResponseMotion.blend', ROOT/'ArtSource/Characters/Candidate03/InfectedModular.blend']
GUARDS = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in GUARD_PATHS}
retained_inventory = retained_validation = retained_samples = None
retained_fbx = {}
retained_action_source = (args.retained_source if args.retained_source and args.retained_source.is_absolute() else ROOT/args.retained_source if args.retained_source else SOURCE/'InfectedMotion.blend').resolve()
if args.recoveries_only:
    retained_inventory = json.loads((SOURCE/'inventory.json').read_text())
    retained_validation = json.loads((SOURCE/'source_validation.json').read_text())
    retained_samples = json.loads((SOURCE/'source_samples.json').read_text())
    for name, definition in retained_inventory['clips'].items():
        if name in RECOVERIES: continue
        p = ROOT/definition['source']
        assert p.is_relative_to(EXPORT) and p.is_file()
        retained_fbx[name] = hashlib.sha256(p.read_bytes()).hexdigest()
        assert retained_fbx[name] == definition['fbx_sha256'], name
    assert len(retained_fbx) == 12 and retained_action_source.is_file()
# These are the same family budgets/contact clocks as ONEInfectedAttackDefinition.
ATTACKS = [('Swipe', .96, .45, 18., .34, .67),
           ('Rake', 1.08, .48, 12., .34, .75),
           ('TwoHand', 1.12, .54, 14., .38, .80)]
assert INPUT.is_file(), 'Generate the new Maintenance character before authoring its motion'
SOURCE.mkdir(parents=True, exist_ok=True)
EXPORT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(INPUT))
scene = bpy.context.scene
rig = bpy.data.objects['Rig_Infected']
assert len(rig.data.bones) == 21, 'Expected the established 21-bone authored infected rig'
assert rig.matrix_world == Matrix.Identity(4), 'Authoring requires the existing identity armature transform'
scene.render.fps = FPS
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01
rig.animation_data_clear()
REST = {b.name: [list(row) for row in b.matrix_local] for b in rig.data.bones}
STANDING_PELVIS_Z = rig.data.bones['pelvis'].head_local.z - 1.4


def surfaces():
    digest = hashlib.sha256()
    for ob in sorted((o for o in bpy.data.objects if o.type == 'MESH'), key=lambda o: o.name):
        digest.update(ob.name.encode())
        for v in ob.data.vertices:
            digest.update(struct.pack('<3f', *v.co))
            for g in v.groups:
                digest.update(struct.pack('<If', g.group, g.weight))
        for p in ob.data.polygons:
            digest.update(struct.pack('<I', p.material_index))
            digest.update(struct.pack('<' + 'I' * len(p.vertices), *p.vertices))
    return digest.hexdigest()


SURFACES = surfaces()
if args.recoveries_only:
    for name in retained_fbx:
        existing = bpy.data.actions.get(name)
        if existing: bpy.data.actions.remove(existing)
    with bpy.data.libraries.load(str(retained_action_source), link=False) as (available, chosen):
        assert set(retained_fbx).issubset(available.actions)
        chosen.actions = list(retained_fbx)
    assert all(bpy.data.actions.get(name) is not None for name in retained_fbx)
    assert all(bpy.data.actions[name].library is None for name in retained_fbx)
    for name in retained_fbx:bpy.data.actions[name].use_fake_user=True
    # Appended actions are local. Remove only the now-unused library record so
    # Blender can save this refreshed scene back to the authoring source path.
    for library in list(bpy.data.libraries):
        if Path(bpy.path.abspath(library.filepath)).resolve()==retained_action_source:
            bpy.data.batch_remove(ids=[library])
    assert all(bpy.data.actions.get(name) is not None and bpy.data.actions[name].library is None for name in retained_fbx)


def smooth(t):
    t = max(0., min(1., t))
    return t * t * (3. - 2. * t)


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


def rotate(name, axis, degrees):
    bone = rig.pose.bones[name]
    matrix = bone.matrix.copy()
    point = matrix.translation.copy()
    bone.matrix = (Matrix.Translation(point) @ Quaternion(Vector(axis), math.radians(degrees)).to_matrix().to_4x4()
                   @ Matrix.Translation(-point) @ matrix)
    bpy.context.view_layer.update()


def segment(name, head, tail):
    rest = rig.data.bones[name]
    rotation = (rest.tail_local - rest.head_local).normalized().rotation_difference((tail - head).normalized())
    matrix = rotation.to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4()
    matrix.translation = head
    rig.pose.bones[name].matrix = matrix
    bpy.context.view_layer.update()


def chain(upper, lower, target, pole):
    hip = rig.pose.bones[upper].head.copy()
    l1, l2 = rig.data.bones[upper].length, rig.data.bones[lower].length
    delta = Vector(target) - hip
    distance = max(.01, min(delta.length, l1 + l2 - .05))
    axis = delta.normalized()
    goal = hip + axis * distance
    along = (l1*l1 - l2*l2 + distance*distance)/(2*distance)
    height = math.sqrt(max(0, l1*l1 - along*along))
    bend = Vector(pole) - hip
    bend = (bend - axis*bend.dot(axis)).normalized()
    knee = hip + axis*along + bend*height
    segment(upper, hip, knee)
    segment(lower, knee, goal)
    return goal


def leg(side, target, yaw=0., pitch=0.):
    hip = rig.pose.bones['thigh_' + side].head
    pole = Vector((80., -10. if side == 'l' else 10., 53.))
    # Keep a stable knee plane even in the initial horizontal recovery pose.
    if abs(Vector(target).x-hip.x) > 45:
        pole = hip + Vector((10., -18. if side == 'l' else 18., 45.))
    goal = chain('thigh_' + side, 'calf_' + side, target, pole)
    rest = rig.data.bones['foot_' + side]
    matrix = (Quaternion(Vector((0, 0, 1)), yaw) @ Quaternion(Vector((0, 1, 0)), pitch)).to_matrix().to_4x4()
    matrix = matrix @ rest.matrix_local.to_3x3().to_4x4()
    matrix.translation = goal
    rig.pose.bones['foot_' + side].matrix = matrix
    bpy.context.view_layer.update()


def arm(side, target, pole=None, hand=(1, 0, -10)):
    sign = -1 if side == 'l' else 1
    goal = chain('upperarm_' + side, 'lowerarm_' + side, target, pole or (6, sign*38, 118))
    segment('hand_' + side, goal, goal + Vector(hand))


def reach_feet(targets):
    drop = 0.
    for side, target, _, _ in targets:
        hip = rig.pose.bones['thigh_' + side].head
        length = rig.data.bones['thigh_' + side].length + rig.data.bones['calf_' + side].length - .3
        horizontal = (target.x-hip.x)**2 + (target.y-hip.y)**2
        drop = min(drop, target.z + math.sqrt(max(0., length*length-horizontal)) - hip.z)
    if drop < 0:
        move('pelvis', (0, 0, drop))
    for side, target, yaw, pitch in targets:
        leg(side, target, yaw, pitch)


def curve(t, keys):
    if t <= keys[0][0]:
        return Vector(keys[0][1])
    for (a, x), (b, y) in zip(keys, keys[1:]):
        if t <= b:
            return Vector(x).lerp(Vector(y), smooth((t-a)/(b-a)))
    return Vector(keys[-1][1])


def hanging(phase=0., effort=0.):
    for side, sign, offset in (('l', -1, 0), ('r', 1, .5)):
        wave = math.sin((phase+offset)*math.tau)
        arm(side, (7 + effort*wave, sign*(26 + .7*wave), 94 + 1.5*wave),
            (5 + .4*effort*wave, sign*36, 120), (2, sign*.6, -10))


def idle(phase):
    breath = math.sin(phase*math.tau)
    move('pelvis', (.4*breath, .5*breath, -1.4))
    rotate('spine_01', (0, 1, 0), 5. + .55*breath)
    rotate('spine_02', (1, 0, 0), 1.2)
    rotate('neck', (0, 1, 0), -2. + .4*breath)
    reach_feet([(s, Vector((0, sign*10.5, 10)), math.radians(sign*4), 0) for s, sign in (('l', -1), ('r', 1))])
    hanging(phase, .7)


def gait(phase, running):
    speed, duration, stance = (195., .66, .38) if running else (100., 1.04, .50)
    excursion = speed*duration*stance
    transfer = math.sin(phase*math.tau)
    move('pelvis', (0, (1.0 if running else 1.45)*transfer,
                    (-2.4 if running else -1.8)+(.7 if running else .45)*math.cos(phase*math.tau*2)))
    rotate('pelvis', (0, 0, 1), 2.2*transfer)
    rotate('spine_01', (0, 1, 0), 7. if running else 5.)
    rotate('spine_01', (1, 0, 0), -1.1*transfer)
    rotate('spine_02', (0, 0, 1), -3.*transfer)
    rotate('neck', (0, 0, 1), 1.5*transfer)
    targets = []
    for side, sign, offset in (('l', -1, 0), ('r', 1, .5)):
        u = (phase+offset) % 1.
        if u < stance:
            travel = excursion*(.5-u/stance)
            lift, pitch = 0., .08*smooth((u/stance-.8)/.2)
        else:
            swing = (u-stance)/(1-stance)
            travel = excursion*(-.5+smooth(swing))
            lift = (12. if running else 7.5)*math.sin(math.pi*swing)**1.3
            pitch = -.08*math.sin(math.pi*swing)
        target = Vector((travel, sign*10.5, 10+lift))
        target.z += max(0., 20*math.sin(pitch), -8*math.sin(pitch)) + 9.4*(math.cos(pitch)-1)
        targets.append((side, target, math.radians(sign*4), pitch))
    reach_feet(targets)
    hanging(phase, 15. if running else 10.)


def turn(phase, direction):
    # A coordinated step-and-settle turn support cycle. Actor yaw remains the
    # movement authority; this clip contains no root rotation or displacement.
    wave = math.sin(phase*math.tau)
    move('pelvis', (0, direction*1.8*wave, -1.8-1.2*math.sin(math.pi*phase)**2))
    rotate('pelvis', (0, 0, 1), direction*5*wave)
    rotate('spine_01', (0, 0, 1), direction*8*wave)
    rotate('spine_01', (0, 1, 0), 5)
    rotate('neck', (0, 0, 1), -direction*5*wave)
    targets = []
    for side, sign, offset in (('l', -1, 0), ('r', 1, .5)):
        u = (phase+offset) % 1.
        lift = 7*math.sin(math.pi*u/.5)**2 if u < .5 else 0
        targets.append((side, Vector((direction*sign*3*wave, sign*(10.5+1.5*abs(wave)), 10+lift)),
                        math.radians(sign*4+direction*10*wave), 0))
    reach_feet(targets)
    hanging(phase, 5.)


def step_position(t, distance, end, entry=195.):
    entry = max(0., min(195., entry))
    end = min(end, 3.*distance/entry) if entry > 0 else end
    u = max(0., min(1., t/end))
    tangent = entry*end/distance
    return distance*((-2*u+3)*u*u + (u*u*u-2*u*u+u)*tangent)


def attack(phase, family, anatomical):
    _, duration, contact, distance, end, recovery = ATTACKS[family]
    t = phase*duration
    source = 'r' if anatomical in ('Left', 'Both') else 'l'
    active_sign = 1 if source == 'r' else -1
    advance = step_position(t, distance, end)
    wind, strike = smooth(t/.22), smooth((t-.22)/(contact-.22))
    release = smooth((t-contact)/(duration-contact))
    load = math.sin(math.pi*phase)
    torso = active_sign*(-19*wind+40*strike)*(1-release) if family != 2 else 0.
    move('pelvis', (2.2*load, active_sign*1.7*load, -1.8-2.5*load*load))
    rotate('pelvis', (0, 0, 1), torso*.23)
    rotate('spine_01', (0, 1, 0), 5+9*strike*(1-release))
    rotate('spine_01', (0, 0, 1), torso*.49)
    rotate('spine_02', (0, 0, 1), torso*.28)
    rotate('neck', (0, 0, 1), -torso*.40)
    targets = []
    for side, sign in (('l', -1), ('r', 1)):
        lead = side == source
        u = max(0., min(1., t/end if lead else (t-contact-.05)/(duration-contact-.05)))
        world_x = (4 if lead else -4) + distance*smooth(u)
        targets.append((side, Vector((world_x-advance, sign*10.5, 10+(8 if lead else 6)*math.sin(math.pi*u))), math.radians(sign*4), 0))
    reach_feet(targets)
    for side, sign in (('l', -1), ('r', 1)):
        rest = (7, sign*26, 94)
        if family != 2 and side != source:
            goal = (7+5*load, sign*(26+3*load), 94+6*load)
        elif family == 0:
            goal = curve(t, [(0, rest), (.22, (-6, sign*36, 129)), (contact, (58, -sign*5, 126)),
                             (contact+.17, (31, -sign*24, 101)), (duration, rest)])
        elif family == 1:
            goal = curve(t, [(0, rest), (.25, (15, -sign*16, 142)), (contact, (55, sign*14, 129)),
                             (contact+.22, (28, sign*38, 104)), (duration, rest)])
        else:
            goal = curve(t, [(0, rest), (.28, (22, sign*17, 145)), (contact, (54, sign*13, 121)),
                             (contact+.20, (33, sign*19, 94)), (duration, rest)])
        arm(side, goal, (12, sign*40, 119), (7*strike*(1-release)+1, -sign*2*load, -10+6*load))


def reaction(phase, stumble=False):
    idle(0)
    pulse = math.sin(math.pi*phase)**1.2
    move('pelvis', ((-2 if not stumble else 5)*pulse, -2*pulse if stumble else 0, (-2 if not stumble else -8)*pulse))
    rotate('spine_01', (0, 1, 0), (17 if stumble else -12)*pulse)
    rotate('spine_02', (0, 0, 1), -7*pulse)
    rotate('neck', (0, 1, 0), -5*pulse)
    targets = [(s, Vector(((13 if sign > 0 else -8)*pulse if stumble else -3*pulse, sign*(10.5+3*pulse), 10+5*math.sin(math.pi*phase)**2)),
                math.radians(sign*4), 0) for s, sign in (('l', -1), ('r', 1))]
    reach_feet(targets)
    for side, sign in (('l', -1), ('r', 1)):
        arm(side, (7+(28 if stumble else 9)*pulse, sign*(26+9*pulse), 94+(12 if stumble else 5)*pulse))


GET_UP_RISE_SECONDS = 2.55
GET_UP_ROLL_SECONDS = .45


def recovery_leg(side, target, pole, roll=0.):
    """Separate pole control prevents the gait solver's long-leg pole switch."""
    goal = chain('thigh_'+side, 'calf_'+side, target, pole)
    rest = rig.data.bones['foot_'+side]
    sign = -1 if side == 'l' else 1
    rotation = Quaternion(Vector((1,0,0)), math.radians(roll)) @ Quaternion(Vector((0,0,1)), math.radians(sign*4))
    matrix = rotation.to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4()
    matrix.translation = goal
    rig.pose.bones['foot_'+side].matrix = matrix
    bpy.context.view_layer.update()
    return (goal-Vector(target)).length


def recovery_step(u, start, end, a, b, lift):
    f = max(0., min(1., (u-a)/(b-a)))
    goal = Vector(start).lerp(Vector(end), smooth(f))
    goal.z += lift*math.sin(math.pi*f)**2
    return goal


def get_up(phase, prone):
    # Runtime holds this first frame during the snapshot blend, then plays the
    # whole authored clip. Supine rolls LOW before the shared supported rise.
    t = phase*(GET_UP_RISE_SECONDS+(0 if prone else GET_UP_ROLL_SECONDS))
    u = max(0., (t-(0 if prone else GET_UP_ROLL_SECONDS))/GET_UP_RISE_SECONDS)
    roll = 0. if prone else 180.*(1-smooth(t/GET_UP_ROLL_SECONDS))
    pelvis = curve(u, [(0,(0,0,25)), (.12,(0,0,25)), (.32,(-15,0,40)),
                       (.52,(-15,0,40)), (.59,(-5,0,40)), (.76,(5,1.5,72)),
                       (.84,(4,3,81)), (.91,(1,-2,89)), (1,(0,0,STANDING_PELVIS_Z))])
    pitch = curve(u, [(0,(87,0,0)), (.12,(87,0,0)), (.32,(75,0,0)),
                      (.59,(75,0,0)), (.76,(30,0,0)), (.91,(8,0,0)), (1,(0,0,0))]).x
    move('pelvis', pelvis-rig.pose.bones['pelvis'].head)
    rotate('pelvis',(0,1,0),pitch)
    rotate('pelvis',(1,0,0),roll)
    finish = smooth((u-.59)/.41)
    rotate('spine_01',(0,1,0),5*finish)
    rotate('spine_02',(1,0,0),1.2*finish)
    rotate('neck',(0,1,0),-2*finish)
    anchors={}; maximum_error=0.
    for side,sign in (('l',-1),('r',1)):
        # Right knee tucks under the trunk; left foot makes a distinct lifted
        # swing. Both targets then remain fixed through the weight transfer.
        if side=='l':
            ankle=recovery_step(u,(-82,sign*10.5,10),(-33,sign*10.5,10),.12,.32,9)
            if u>=.80:ankle=recovery_step(u,(-33,sign*10.5,10),(0,sign*10.5,10),.80,.91,11)
            pole=Vector((50,sign*13,-30))
            planted=(.32<=u<=.80) or u>=.91
        else:
            ankle=recovery_step(u,(-82,sign*10.5,10),(15,sign*10.5,10),.32,.51,14)
            swing=max(0.,min(1.,(u-.32)/.19))
            ankle.y+=sign*18*math.sin(math.pi*swing)**2
            if u>=.92:ankle=recovery_step(u,(15,sign*10.5,10),(0,sign*10.5,10),.92,1.,6)
            # Bring the boot around the outside of the planted knee. An upward
            # pole while the ankle passes close to the hip folded the knee
            # above the back in the first source preview despite valid lengths.
            pole=Vector((-35,sign*14,-10)).lerp(Vector((55,sign*45,10)),smooth((u-.24)/.12))
            pole=pole.lerp(Vector((80,sign*10,53)),smooth((u-.46)/.10))
            planted=(.51<=u<=.92) or u>=1
        if roll>0:
            ankle.y=sign*10.5*math.cos(math.radians(roll))
            ankle.z=10+max(0.,sign*10.5*math.sin(math.radians(roll)))
            pole=Vector((-35,sign*25*math.cos(math.radians(roll)),25-35*math.cos(math.radians(roll))))
        maximum_error=max(maximum_error,recovery_leg(side,ankle,pole,roll))
        if planted and roll==0:anchors['foot_'+side]=list(ankle)
        plant=Vector((42,sign*28,2.4))
        release=smooth((u-(.56 if side=='r' else .59))/(.11 if side=='r' else .08))
        shoulder=rig.pose.bones['upperarm_'+side].head
        free=shoulder+Vector((3,sign*8,-43))
        free=free.lerp(Vector((7,sign*26,94)),smooth((u-.76)/.24))
        goal=plant.lerp(free,release)
        if roll>0:
            a=math.radians(roll)
            goal=Vector((42,sign*28*math.cos(a)+22.6*math.sin(a),max(2.4,25+sign*28*math.sin(a)-22.6*math.cos(a))))
        hand=Vector((10,0,.8)).lerp(Vector((2,sign*.6,-10)),release)
        arm(side,goal,(goal.x-16,sign*43,max(14,goal.z+20)),hand)
        maximum_error=max(maximum_error,(rig.pose.bones['hand_'+side].head-goal).length)
        if release==0 and roll==0:anchors['hand_'+side]=list(plant)
    label='roll_to_prone' if roll>0 else next(label for end,label in [(.12,'palm_brace'),(.32,'right_knee_tuck'),(.51,'left_foot_swing'),(.59,'planted_weight_transfer'),(.80,'supported_rise'),(.91,'right_foot_step'),(1.001,'left_foot_settle')] if u<=end)
    assert maximum_error<.15, ('Recovery unreachable support target',phase,prone,maximum_error)
    return {'recovery_phase':label,'rise_phase':u,'roll_degrees':roll,'support_anchors':anchors,'maximum_target_error_cm':maximum_error}


inventory = {'candidate': '07', 'fps': FPS, 'destination': '/Game/ONE/Animations/Candidate07',
             'source': INPUT.relative_to(ROOT).as_posix(), 'source_sha256': hashlib.sha256(INPUT.read_bytes()).hexdigest(),
             'source_axes': '+X forward; source _r anatomical LEFT, _l anatomical RIGHT; imported skeleton has armature root',
             'authorship': 'Original Project ONE authored skeletal poses; existing local IK/FBX conventions reused. No third-party animation.',
             'clips': {}, 'limits': 'Authoring/invariant results are separate from UE import, evaluated runtime motion, contact, and visual approval.'}
validation = {'scope': 'Source rig/surface preservation and kinematic checks only', 'clips': {}}
all_samples = {}
if args.recoveries_only:
    retained_source_manifest_path=retained_action_source.parent/'inventory.json'
    retained_source_manifest=json.loads(retained_source_manifest_path.read_text())
    for name,digest in retained_fbx.items():
        assert retained_source_manifest['clips'][name]['fbx_sha256']==digest, ('Preserved action-source inventory mismatch',name)
    inventory['retained_action_source']={'description':'Previously authored C07 InfectedMotion.blend supplies the twelve unchanged actions; exported FBX identities remain authoritative in clips.',
        'bytes':retained_action_source.stat().st_size,'sha256':hashlib.sha256(retained_action_source.read_bytes()).hexdigest(),
        'source_inventory_sha256':hashlib.sha256(retained_source_manifest_path.read_bytes()).hexdigest(),
        'recorded_character_source':retained_source_manifest['source'],'recorded_character_source_sha256':retained_source_manifest['source_sha256']}
    rig.animation_data_create()
    for name in retained_fbx:
        rig.animation_data.action=bpy.data.actions[name]
        maximum_error=0.
        for row in retained_samples[name]:
            scene.frame_set(round(row['time']*FPS)+1)
            for joint in ('pelvis','head','hand_l','hand_r','calf_l','calf_r','foot_l','foot_r'):
                maximum_error=max(maximum_error,(rig.pose.bones[joint].head-Vector(row[joint])).length)
        assert maximum_error<.002, ('Retained action evaluated pose changed',name,maximum_error)
        inventory['clips'][name]=retained_inventory['clips'][name]
        validation['clips'][name]={**retained_validation['clips'][name],'retained_fbx_unchanged':True,
                                   'refreshed_blend_pose_maximum_error_cm':maximum_error}
        all_samples[name]=retained_samples[name]


def export(key, duration, pose, extra=None, loop=False):
    name = 'A_Infected_C07_' + key
    previous = bpy.data.actions.get(name)
    if previous:
        bpy.data.actions.remove(previous)
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = action
    count = round(duration*FPS)
    scene.frame_start, scene.frame_end = 1, count+1
    samples = []
    for i in range(count+1):
        scene.frame_set(i+1)
        reset()
        evidence=pose(i/count)
        for bone in rig.pose.bones:
            assert all(math.isfinite(v) for row in bone.matrix for v in row), (name, i, bone.name)
            for channel in ('location', 'rotation_quaternion', 'scale'):
                bone.keyframe_insert(channel, frame=i+1, group=bone.name)
        row = {'time': i/FPS}
        for joint in ('pelvis', 'head', 'hand_l', 'hand_r', 'calf_l', 'calf_r', 'foot_l', 'foot_r'):
            row[joint] = list(rig.pose.bones[joint].head)
        row['chain_gap_cm'] = max((rig.pose.bones[a+s].tail-rig.pose.bones[b+s].head).length
                                  for a, b in [('thigh_', 'calf_'), ('calf_', 'foot_'), ('upperarm_', 'lowerarm_'), ('lowerarm_', 'hand_')]
                                  for s in ('l', 'r'))
        assert row['chain_gap_cm'] < .01, (name, i, row['chain_gap_cm'])
        if evidence:
            row.update(evidence)
            row['maximum_support_anchor_error_cm']=max(((Vector(row[j])-Vector(p)).length for j,p in evidence['support_anchors'].items()),default=0.)
            assert row['maximum_support_anchor_error_cm']<.05, (name,i,'Support anchor drift',row['maximum_support_anchor_error_cm'])
        samples.append(row)
    if loop:
        gap = max((Vector(samples[0][j])-Vector(samples[-1][j])).length for j in ('pelvis','hand_l','hand_r','foot_l','foot_r'))
        assert gap < .01, (name, 'loop endpoint gap', gap)
    scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    output = EXPORT / (name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(output), use_selection=True, object_types={'ARMATURE'},
        axis_forward='-Y', axis_up='Z', global_scale=1, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        use_space_transform=True, bake_space_transform=False, add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
        use_armature_deform_only=False, bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0, path_mode='STRIP')
    inventory['clips'][name] = {'key': key, 'duration': duration, 'loop': loop, 'skeleton': 'SK_Infected_Skeleton',
                                'source': output.relative_to(ROOT).as_posix(), 'fbx_sha256': hashlib.sha256(output.read_bytes()).hexdigest(), **(extra or {})}
    validation['clips'][name] = {'samples': len(samples), 'maximum_chain_gap_cm': max(s['chain_gap_cm'] for s in samples),
                                 'head_min_z_cm': min(s['head'][2] for s in samples), 'head_max_z_cm': max(s['head'][2] for s in samples)}
    if key.startswith('GetUp'):
        support=[s for s in samples if s['recovery_phase']=='planted_weight_transfer']
        assert support and all(all(k in s['support_anchors'] for k in ('hand_l','foot_l','foot_r')) for s in support if s['rise_phase']<=.56)
        knee=[s['calf_l'][2] for s in samples if .33<=s['rise_phase']<=.51]
        assert knee and min(knee)>3 and max(knee)<10, (name,'Rear knee support height',min(knee),max(knee))
        lead_swing=[s for s in samples if .36<=s['rise_phase']<=.49]
        assert lead_swing and max(s['calf_r'][2]-s['pelvis'][2] for s in lead_swing)<20, (name,'Lead knee folds above back')
        rolling=[s for s in samples if s['recovery_phase']=='roll_to_prone']
        if rolling:assert max(s['pelvis'][2] for s in rolling)-min(s['pelvis'][2] for s in rolling)<.01
        validation['clips'][name].update(maximum_support_anchor_error_cm=max(s['maximum_support_anchor_error_cm'] for s in samples),
            maximum_target_error_cm=max(s['maximum_target_error_cm'] for s in samples),rear_knee_support_z_cm=[min(knee),max(knee)],
            support_samples=sum(bool(s['support_anchors']) for s in samples),low_roll_pelvis_z_cm=[min(s['pelvis'][2] for s in rolling),max(s['pelvis'][2] for s in rolling)] if rolling else None)
    all_samples[name] = samples


if not args.recoveries_only:
    export('Idle', 2.4, idle, loop=True)
    export('Walk', 1.04, lambda p: gait(p, False), {'speed': 100, 'stance_fraction': .50, 'stride_cm': 104}, True)
    export('Run', .66, lambda p: gait(p, True), {'speed': 195, 'stance_fraction': .38, 'stride_cm': 128.7}, True)
    export('TurnLeft', 1., lambda p: turn(p, 1), loop=True)
    export('TurnRight', 1., lambda p: turn(p, -1), loop=True)
    for family, (key, duration, contact, distance, end, recovery) in enumerate(ATTACKS):
        for side in (('Both',) if family == 2 else ('Left', 'Right')):
            export(key + (side if family != 2 else ''), duration, lambda p, f=family, s=side: attack(p, f, s),
                   {'family': family, 'required_limbs': side, 'contact_time': contact, 'step_distance_cm': distance,
                    'step_end': end, 'authored_entry_speed_cm_s': 195, 'recovery_locomotion_start': recovery,
                    'foot_correction': 'Runtime position-space leg IK subtracts actual capsule travel from the same reference Hermite advance.'})
    export('HeavyHit', .52, lambda p: reaction(p, False))
    export('Stumble', .70, lambda p: reaction(p, True))
for prone in (True,False):
    export('GetUpProne' if prone else 'GetUpSupine', GET_UP_RISE_SECONDS+(0 if prone else GET_UP_ROLL_SECONDS), lambda p,prone=prone:get_up(p,prone),
        {'recovery_phases':['low_roll' if not prone else 'prone_entry','palm_brace','right_knee_tuck','left_foot_swing','planted_weight_transfer','supported_rise','right_foot_step','left_foot_settle'],
         'support_contract':'Stationary component-space palm frames during brace/tuck/lead-foot plant; stationary planted foot targets through weight transfer. Separate lifted adjustment steps before standing. Runtime snapshot blend precedes this clip clock.'})
assert REST == {b.name: [list(row) for row in b.matrix_local] for b in rig.data.bones}, 'Bind matrices changed'
assert SURFACES == surfaces(), 'Character meshes/weights/material assignments changed'
validation.update(result='PASS', source_bones=len(REST), bind_matrices_unchanged=True, surfaces_unchanged=True,
                  surface_digest=SURFACES, source_bind_sha256=hashlib.sha256(json.dumps(REST, sort_keys=True).encode()).hexdigest())
for name,digest in retained_fbx.items():assert hashlib.sha256((ROOT/inventory['clips'][name]['source']).read_bytes()).hexdigest()==digest
assert all(bpy.data.actions[name].use_fake_user for name in inventory['clips']), 'All fourteen clips must persist without an active rig user'
for path,digest in GUARDS.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==digest, path.name
validation['source_guards']={p.relative_to(ROOT).as_posix():h for p,h in GUARDS.items()}
validation['retained_clips']=list(retained_fbx)
rig.animation_data.action = bpy.data.actions['A_Infected_C07_Idle']
scene.frame_set(1)
scene.render.filepath = '//MotionPreview.png'
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type == 'FILE_BROWSER' and area.spaces.active.params:
            area.spaces.active.params.directory = b'//'
# Resolve current character textures before moving the editable scene into its
# Motion subfolder. Only stored paths change; original pixels/materials remain.
for image in bpy.data.images:
    if image.source=='FILE' and image.filepath:
        original=Path(bpy.path.abspath(image.filepath)).resolve()
        assert original.is_file() and original.parent==SOURCE.parent, image.filepath
        image.filepath='//../'+original.name
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'InfectedMotion.blend'),relative_remap=False)
# A local action can exist in memory yet be discarded at save if it has no
# user. Reopen the actual written blend and verify all fourteen survive, bind
# to the refreshed rig and reproduce sampled source poses before claiming PASS.
bpy.ops.wm.open_mainfile(filepath=str(SOURCE/'InfectedMotion.blend'))
scene=bpy.context.scene;rig=bpy.data.objects['Rig_Infected']
assert SURFACES==surfaces(), 'Saved/reloaded character surface changed'
assert REST=={b.name:[list(row) for row in b.matrix_local] for b in rig.data.bones}
reload_error=0.;reload_slots={}
for name in inventory['clips']:
    action=bpy.data.actions.get(name)
    assert action is not None and action.use_fake_user, ('Saved action missing/unretained',name)
    rig.animation_data.action=action
    assert rig.animation_data.action_slot is not None, ('Saved action slot unbound',name)
    reload_slots[name]=rig.animation_data.action_slot.identifier
    rows=all_samples[name]
    for index in (0,len(rows)//2,len(rows)-1):
        row=rows[index];scene.frame_set(round(row['time']*FPS)+1)
        for joint in ('pelvis','head','hand_l','hand_r','calf_l','calf_r','foot_l','foot_r'):
            reload_error=max(reload_error,(rig.pose.bones[joint].head-Vector(row[joint])).length)
assert reload_error<.002, ('Saved/reloaded action pose changed',reload_error)
for image in bpy.data.images:
    if image.source=='FILE' and image.filepath:
        resolved=Path(bpy.path.abspath(image.filepath)).resolve()
        assert resolved.is_file() and resolved.parent==SOURCE.parent, image.filepath
validation.update(saved_blend_reload_actions=sorted(inventory['clips']),maximum_reload_pose_error_cm=reload_error,saved_action_slots=reload_slots)
(SOURCE/'inventory.json').write_text(json.dumps(inventory, indent=2)+'\n', encoding='utf-8')
(SOURCE/'source_validation.json').write_text(json.dumps(validation, indent=2)+'\n', encoding='utf-8')
(SOURCE/'source_samples.json').write_text(json.dumps(all_samples, separators=(',', ':'))+'\n', encoding='utf-8')
print('CANDIDATE07_INFECTED_MOTION_AUTHORED ' + str(len(inventory['clips'])))
if args.render_recovery:
    # Optional bounded source preview, separate from the editable character and
    # numeric evidence. Actual runtime recovery is still the integration gate.
    output=ROOT/'Saved/Candidate07/CharacterAudit/RecoverySourceR3'
    output.mkdir(parents=True,exist_ok=True)
    for ob in list(bpy.data.objects):
        if ob.type in ('CAMERA','LIGHT'):bpy.data.objects.remove(ob,do_unlink=True)
    bpy.ops.mesh.primitive_plane_add(size=700,location=(0,0,-.1))
    floor=bpy.context.object;floor.name='PreviewFloor'
    material=bpy.data.materials.new('RecoveryPreviewFloor');material.diffuse_color=(.16,.18,.19,1);floor.data.materials.append(material)
    bpy.ops.object.camera_add(location=(225,-280,185))
    camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,85))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=260;scene.camera=camera
    for location,power,size in [((120,-150,250),1250,180),((-100,80,190),1000,160)]:
        bpy.ops.object.light_add(type='AREA',location=location);light=bpy.context.object;light.data.energy=power*300;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(Vector((0,0,70))-light.location).to_track_quat('-Z','Y').to_euler()
    scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.13,.145,.16,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.65
    scene.render.engine='CYCLES';scene.cycles.samples=12;scene.cycles.use_denoising=True;scene.render.threads_mode='FIXED';scene.render.threads=8
    scene.render.resolution_x=800;scene.render.resolution_y=600;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
    preview=[]
    for prone in (True,False):
        name='A_Infected_C07_GetUpProne' if prone else 'A_Infected_C07_GetUpSupine';rig.animation_data.action=bpy.data.actions[name]
        duration=inventory['clips'][name]['duration'];times=([0,.30,.65,1.02,1.40,1.68,2.10,2.55] if prone else [0,.225,.45,.80,1.35,1.85,2.35,3.0])
        for index,t in enumerate(times):
            scene.frame_set(round(t*FPS)+1);file=output/(name+'_'+str(index).zfill(2)+'.png');scene.render.filepath=str(file);bpy.ops.render.render(write_still=True)
            preview.append({'clip':name,'seconds':t,'frame':round(t*FPS)+1,'path':file.relative_to(ROOT).as_posix(),'sha256':hashlib.sha256(file.read_bytes()).hexdigest()})
    (output/'inventory.json').write_text(json.dumps({'scope':'Chronological source-pose stills; no runtime or continuous playback claim.','source_sha256':inventory['source_sha256'],'frames':preview},indent=2)+'\n')
