"""Original Candidate02 weapons, existing-rig actions and local sound design.

blender --background --python Scripts/create_candidate02_weapon_assets.py
Blender 5.1.2. Reads the accepted Project ONE sources; never regenerates characters.
All new static exports are grip-centred centimetres, +X forward, +Z up.
New clips use 100 Hz so event boundaries expressed in hundredths remain exact.
"""
from pathlib import Path
import math, json, random, struct, wave, sys

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'ArtSource'/'Weapons'/'Candidate02'
EXPORT=ROOT/'ArtSource'/'Exports'
AUDIO=ROOT/'ArtSource'/'Audio'/'Candidate02'
for directory in (SRC,EXPORT,AUDIO): directory.mkdir(parents=True,exist_ok=True)
FPS=100

def create_audio():
    """Original dry layers + deterministic nonharmonic resonances, no sample inputs."""
    rate=48000
    inventory={}
    def lowpass(values,cutoff):
        alpha=1-math.exp(-2*math.pi*cutoff/rate); state=0.; out=[]
        for value in values: state+=alpha*(value-state); out.append(state)
        return out
    def noise(seconds,seed):
        rng=random.Random(seed)
        return [rng.uniform(-1,1) for _ in range(round(seconds*rate))]
    def band(values,lo,hi):
        high=lowpass(values,hi); lower=lowpass(high,lo)
        return [a-b for a,b in zip(high,lower)]
    def mix(dest,src,start,gain=1):
        offset=round(start*rate)
        for i,v in enumerate(src):
            if 0<=offset+i<len(dest): dest[offset+i]+=v*gain
    def envelope(values,attack,decay):
        return [v*(1-math.exp(-(i/rate)/attack))*math.exp(-(i/rate)/decay) for i,v in enumerate(values)]
    def metal(seconds,seed,weight=1):
        # Inharmonic damped modes model short contact between small machined parts.
        rng=random.Random(seed); n=round(seconds*rate)
        out=envelope(band(noise(seconds,seed),550,6500),.00025,.006)
        modes=[(530,.020),(937,.017),(1663,.012),(2819,.009),(4217,.006)]
        for k,(f,decay) in enumerate(modes):
            frequency=f*rng.uniform(.94,1.06)
            for i in range(n):
                t=i/rate
                out[i]+=weight*.10/(1+k*.3)*math.sin(2*math.pi*frequency*t)*math.exp(-t/decay)
        return out
    def save(name,samples,peak_db,role):
        dc=sum(samples)/max(1,len(samples)); samples=[x-dc for x in samples]
        # Remove sub-audible drift without adding a sustained synthetic tone.
        low=lowpass(samples,32); samples=[x-y for x,y in zip(samples,low)]
        fade=round(.006*rate)
        for i in range(min(fade,len(samples))): samples[-i-1]*=i/fade
        peak=max(max(abs(x) for x in samples),1e-9); target=10**(peak_db/20)
        samples=[math.tanh(x/peak*1.35)/math.tanh(1.35)*target for x in samples]
        pcm=b''.join(struct.pack('<h',round(max(-1,min(1,x))*32767)) for x in samples)
        with wave.open(str(AUDIO/(name+'.wav')),'wb') as output:
            output.setnchannels(1); output.setsampwidth(2); output.setframerate(rate); output.writeframes(pcm)
        rms=math.sqrt(sum(x*x for x in samples)/len(samples))
        inventory[name]={'source':'ArtSource/Audio/Candidate02/'+name+'.wav','asset':'/Game/ONE/Audio/Weapons/'+name,
            'duration':len(samples)/rate,'sample_rate':rate,'peak_dbfs':20*math.log10(max(abs(x) for x in samples)),
            'rms_dbfs':20*math.log10(max(rms,1e-10)),'role':role,'provenance':'Original deterministic local synthesis; no recordings or third-party samples'}
    def shot(name,seed,shotgun=False):
        duration=.74 if shotgun else .43
        result=[0.]*round(duration*rate)
        dry=noise(duration,seed)
        crack=envelope(band(dry,900 if shotgun else 1550,11500),.00012,.009 if shotgun else .006)
        pressure=envelope(band(noise(duration,seed+7),45,380 if shotgun else 640),.0007,.034 if shotgun else .019)
        grit=envelope(band(noise(duration,seed+17),170,2600),.0003,.044 if shotgun else .021)
        mix(result,crack,0,.9);mix(result,pressure,.001,5.0 if shotgun else 2.6);mix(result,grit,.003,1.8 if shotgun else 1.0)
        # Very short muzzle pressure displacement; one transient, not a beep or bass loop.
        for i in range(len(result)):
            t=i/rate; result[i]+=(.55 if shotgun else .25)*(1-t/.009)*math.exp(-t/.006)
        dry_shot=result[:]
        for delay,gain in ([(.027,.16),(.061,.09),(.119,.045),(.183,.025)] if shotgun else [(.019,.11),(.042,.055),(.083,.027)]):
            mix(result,lowpass(dry_shot,3600),delay,gain)
        if not shotgun: mix(result,metal(.13,seed+81),.051,.20)
        save(name,result,-3.0,'Shotgun broad low pressure / longer industrial decay' if shotgun else 'Carbine short high crack / tight cycling action')
    for i in range(1,4):
        shot('S_CarbineShot_%02d'%i,2700+i)
        shot('S_ShotgunShot_%02d'%i,3700+i,True)
    def mechanism(name,seconds,events,seed,peak=-12,slide=None,role='Mechanical event'):
        result=[0.]*round(seconds*rate)
        for j,(start,gain) in enumerate(events): mix(result,metal(.15,seed+j,1),start,gain)
        if slide:
            start,length,gain=slide
            values=band(noise(length,seed+41),260,4500)
            values=[v*math.sin(math.pi*i/max(1,len(values)-1))**.6*(.6+.4*math.sin(i/rate*2*math.pi*87)**2) for i,v in enumerate(values)]
            mix(result,values,start,gain)
        save(name,result,peak,role)
    mechanism('S_ShotgunPumpBack',.23,[(.002,.9),(.159,.65),(.193,.35)],5101,-10.5,(.018,.15,.26),'Pump unlock and rearward rail scrape; play at pump phase 0')
    mechanism('S_ShotgunPumpForward',.25,[(.012,.35),(.19,.75)],5107,-11,(.02,.17,.23),'Forward slide; play at pump phase .21')
    mechanism('S_ShotgunPumpLock',.17,[(.001,1),(.032,.27)],5111,-10,'','Pump chamber lock; play at pump phase .44')
    mechanism('S_ShotgunShellInsert',.19,[(.003,.75),(.041,.48),(.079,.16)],5121,-13,(.005,.07,.14),'Single earned shell insertion; play at shell phase .60')
    mechanism('S_ShotgunReloadStart',.23,[(.027,.35)],5125,-17,(.003,.17,.25),'Cloth/receiver handling into shell loading posture')
    mechanism('S_ShotgunReloadEnd',.23,[(.07,.4),(.125,.2)],5127,-16,(.005,.12,.18),'Return support hand to fore-end')
    mechanism('S_CarbineMagOut',.28,[(.002,.85),(.083,.32)],5201,-12,(.02,.15,.15),'Magazine release at reload .40')
    mechanism('S_CarbineMagIn',.23,[(.003,1),(.044,.33)],5203,-10.5,(.005,.05,.1),'Magazine seating at reload 1.20')
    mechanism('S_CarbineBolt',.26,[(.002,.65),(.067,1),(.11,.22)],5207,-11,(.006,.07,.2),'Bolt manipulation at reload 1.74')
    mechanism('S_WeaponEquip',.20,[(.025,.42),(.095,.20)],5221,-16,(.001,.13,.22),'Short restrained weapon/strap handling at equip swap')
    mechanism('S_CarbineEmpty',.10,[(.001,.75),(.022,.14)],5231,-15,None,'Carbine empty mechanical click')
    mechanism('S_ShotgunEmpty',.13,[(.001,.9),(.038,.18)],5237,-14.5,None,'Shotgun empty hammer action')
    for i in range(1,4):
        vals=[0.]*round(.23*rate)
        mix(vals,envelope(band(noise(.23,6100+i),85,980),.0008,.018),0,3)
        mix(vals,envelope(band(noise(.23,6200+i),1200,6200),.0001,.008),.003,.34)
        save('S_FleshImpact_%02d'%i,vals,-11,'Short wet/dull tissue impact, kept below gunshots')
    for i in range(1,3):
        vals=envelope(band(noise(.24,6300+i),600,7600),.0002,.024)
        mix(vals,metal(.11,6400+i),.001,.18)
        save('S_ConcreteImpact_%02d'%i,vals,-13,'Dry mineral impact and granular scatter')
        vals=metal(.33,6500+i,2.2);mix(vals,metal(.15,6600+i),.039,.13)
        save('S_MetalImpact_%02d'%i,vals,-13,'Short inharmonic metal impact')
    (AUDIO/'inventory.json').write_text(json.dumps({'status':'Original authored candidate audio; perceptual quality must be reviewed in gameplay','events':inventory},indent=2))
    return inventory

if '--audio-only' in sys.argv:
    create_audio(); print('CANDIDATE02_AUDIO_COMPLETE'); raise SystemExit(0)

import bpy
from mathutils import Vector,Matrix,Quaternion

def preserve_action_durations(original_fps):
    # Retain editable C01 reference actions at their original duration in the 100Hz source.
    for action in bpy.data.actions:
        for layer in action.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    for curve in bag.fcurves:
                        for key in curve.keyframe_points:
                            key.co.x=1+(key.co.x-1)*FPS/original_fps
                            key.handle_left.x=1+(key.handle_left.x-1)*FPS/original_fps
                            key.handle_right.x=1+(key.handle_right.x-1)*FPS/original_fps

def load_response():
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/'ArtSource'/'Characters'/'Response.blend'))
    scene=bpy.context.scene
    preserve_action_durations(scene.render.fps)
    scene.render.fps=FPS;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
    rig=bpy.data.objects['Rig_Response']; rig.location=(0,0,0);rig.animation_data_clear()
    return scene,rig

scene,rig=load_response()
palette=json.loads((ROOT/'ArtSource'/'Environment'/'manifest.json').read_text())['palette']
palette.update({'M_ShotgunPolymer':[[.11,.076,.047,1],.86,0], 'M_ShellHull':[[.39,.075,.045,1],.65,0], 'M_Brass':[[.46,.31,.095,1],.4,.75]})
materials={}
for name,(color,rough,metal) in palette.items():
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=color
    shader=m.node_tree.nodes.get('Principled BSDF');shader.inputs['Base Color'].default_value=color
    shader.inputs['Roughness'].default_value=rough;shader.inputs['Metallic'].default_value=metal
    materials[name]=m
parts=[]
def finish(o,name,material,bevel=.15):
    o.name=name;o.data.materials.append(materials[material])
    bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Machined chamfer','BEVEL');mod.width=bevel;mod.segments=3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(o);return o
def box(name,loc,size,material,bevel=.2,rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;o.rotation_euler=rot
    return finish(o,name,material,bevel)
def cylinder(name,loc,radius,length,material,axis='X',vertices=32,bevel=.1):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=length,location=loc)
    o=bpy.context.object
    if axis=='X':o.rotation_euler.y=math.pi/2
    elif axis=='Y':o.rotation_euler.x=math.pi/2
    return finish(o,name,material,bevel)
def beam(name,a,b,radius,material):
    a,b=Vector(a),Vector(b);mid=(a+b)*.5;delta=b-a
    bpy.ops.mesh.primitive_cylinder_add(vertices=12,radius=radius,depth=delta.length,location=mid)
    o=bpy.context.object;o.rotation_euler=delta.to_track_quat('Z','Y').to_euler();return finish(o,name,material,.06)
def export_fbx(path,objects,animation=False):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:obj.hide_set(False);obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','ARMATURE'},
        axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
        use_space_transform=True,bake_space_transform=False,add_leaf_bones=False,
        primary_bone_axis='Y',secondary_bone_axis='X',use_armature_deform_only=False,
        mesh_smooth_type='FACE',use_mesh_modifiers=True,bake_anim=animation,
        bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,bake_anim_simplify_factor=0)
def commit_mesh(name):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts:obj.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    color=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for cell in color.data:cell.color=(1,1,1,1)
    parts=[];export_fbx(EXPORT/(name+'.fbx'),[o]);return o

# Pump-action shotgun: purposeful receiver, twin tubes, ribbed sliding fore-end,
# exposed ejection/loading ports, simple sights and a stock distinct from the carbine.
box('Forged receiver',(3,0,12),(25,7.4,9),'M_Graphite',1.0)
box('Receiver lower web',(3,0,7),(20,6.4,5),'M_Steel',.5)
cylinder('Heavy barrel',(37,0,14),1.35,53,'M_Steel',vertices=40)
cylinder('Barrel muzzle',(63.8,0,14),1.58,1.2,'M_Graphite',vertices=40)
cylinder('Dark bore',(64.45,0,14),1.08,.10,'M_Rubber',vertices=32,bevel=0)
cylinder('Magazine tube',(29,0,7),1.75,44,'M_Steel',vertices=32)
cylinder('Magazine end cap',(51.6,0,7),2.05,1.8,'M_Graphite',vertices=24)
box('Barrel clamp',(48,0,10.5),(1.7,4.2,8.3),'M_Graphite',.45)
for y in [-2.25,2.25]:cylinder('Clamp fastener',(48,y,10.5),.57,.3,'M_PaleMetal','Y',12,.05)
box('Ejection opening',(5,3.79,13.5),(8.9,.20,3.1),'M_Rubber',.35)
box('Visible bolt',(5,3.94,13.4),(6.8,.15,2.0),'M_Steel',.18)
box('Loading port',(8,0,4.42),(8.4,4.7,.18),'M_Rubber',.3)
box('Loading gate',(7.8,0,4.24),(6.8,3.7,.16),'M_Steel',.2)
box('Pistol grip',(-1.6,0,-.9),(6,5.5,13.5),'M_ShotgunPolymer',.8,rot=(0,-.24,0))
for i in range(6):box('Grip checkering',(-1.5,0,-5.7+i*1.5),(6.1,5.65,.35),'M_Graphite',.1,rot=(0,-.24,0))
beam('Guard rear',(-2,0,4),(2,0,-1.7),.7,'M_Graphite')
beam('Guard lower',(2,0,-1.7),(9,0,-1.7),.7,'M_Graphite')
beam('Guard front',(9,0,-1.7),(10,0,5),.7,'M_Graphite')
beam('Trigger',(4.5,0,4),(3.9,0,.3),.45,'M_Steel')
cylinder('Stock neck',(-15,0,12),2.3,13,'M_Graphite',vertices=24)
box('Fixed stock',(-28,0,10),(23,7.2,12),'M_ShotgunPolymer',1.2)
box('Stock cheek comb',(-25,0,16.7),(17,6.7,2.5),'M_Graphite',.8)
box('Butt recoil pad',(-40,0,8.7),(2.5,8.1,17),'M_Rubber',.8)
for z in [3.0,5.8,8.6,11.4,14.2]:box('Buttpad traction',(-41.3,0,z),(.25,7.3,.65),'M_Graphite',.1)
box('Rear sight base',(-3.8,0,17.3),(5.6,3.3,1.4),'M_Steel',.2)
for y in [-1.25,1.25]:box('Rear notch wing',(-4,y,19),(1.1,.75,2.4),'M_Graphite',.18)
box('Front sight ramp',(57,0,16.8),(4.8,2.0,2.1),'M_Graphite',.3)
cylinder('Front bead',(58.2,0,18.15),.34,.45,'M_Amber','Z',12,.03)
for y in [-3.81,3.81]:
    for x in [-4,11]:cylinder('Receiver pin',(x,y,10.9),.48,.24,'M_Steel','Y',12,.03)
box('Safety tang',(-8.8,0,17.15),(2.8,1.2,.55),'M_Steel',.14)
box('Safety index',(-8.9,0,17.55),(.65,.7,.10),'M_Amber',.02)
for x in [1,2.4,3.8]:box('Receiver identity tally',(x,-3.79,13.8),(.5,.12,2.8),'M_Amber',.03)
shotgun=commit_mesh('SM_PumpShotgun')
box('Pump core',(18,0,6),(17.4,7.8,6.4),'M_ShotgunPolymer',1.1)
for x in [10.3,12.2,14.1,16,17.9,19.8,21.7,23.6,25.5]:
    box('Pump raised rib',(x,0,5.7),(.85,8.4,6.5),'M_Graphite',.28)
for y in [-4.32,4.32]:box('Fore-end identification inset',(18,y,6),(6.7,.14,2.1),'M_ShotgunPolymer',.22)
pump=commit_mesh('SM_PumpShotgun_ForeEnd')
cylinder('Shell polymer hull',(.35,0,0),.97,5.3,'M_ShellHull',vertices=28)
cylinder('Brass shell head',(-2.60,0,0),1.02,.65,'M_Brass',vertices=28)
cylinder('Shell rim',(-2.96,0,0),1.10,.14,'M_Brass',vertices=28,bevel=.03)
cylinder('Primer',(-3.05,0,0),.30,.05,'M_Steel',vertices=16,bevel=.01)
shell=commit_mesh('SM_ShotgunShell')

# Append the accepted carbine, then export an exact body/magazine split for C02.
with bpy.data.libraries.load(str(ROOT/'ArtSource'/'Environment'/'ProjectONE_IndustrialKit.blend'),link=False) as (available,target):
    target.objects=['SM_Carbine']
carbine=target.objects[0];bpy.context.collection.objects.link(carbine);carbine.hide_set(False);carbine.hide_render=False
for i,material in enumerate(carbine.data.materials):
    base=material.name.rsplit('.',1)[0] if material.name.rsplit('.',1)[-1].isdigit() else material.name
    if base in materials:carbine.data.materials[i]=materials[base]
# Split the accepted authored magazine by connected geometry islands. Everything
# else is copied exactly; the original carbine asset and source remain untouched.
parent=list(range(len(carbine.data.vertices)))
def root(index):
    while parent[index]!=index:parent[index]=parent[parent[index]];index=parent[index]
    return index
for face in carbine.data.polygons:
    first=root(face.vertices[0])
    for vertex in face.vertices[1:]:parent[root(vertex)]=first
groups={}
for vertex in carbine.data.vertices:groups.setdefault(root(vertex.index),[]).append(vertex.co.copy())
magazine_groups=set()
for group,vertices in groups.items():
    centre=sum(vertices,Vector())/len(vertices);low=min(v.z for v in vertices);high=max(v.z for v in vertices)
    if 8<centre.x<20 and high<7.4 and (low<-9 or (centre.x>10 and -10<centre.z<-1 and abs(centre.y)>1.7)):
        magazine_groups.add(group)
if len(magazine_groups)!=4:raise RuntimeError('Expected accepted magazine and three ribs; found '+str(len(magazine_groups)))
def copy_faces(name,want_magazine):
    faces=[p for p in carbine.data.polygons if (root(p.vertices[0]) in magazine_groups)==want_magazine]
    indices=sorted({i for p in faces for i in p.vertices});mapping={old:new for new,old in enumerate(indices)}
    data=bpy.data.meshes.new(name+'_Mesh');data.from_pydata([carbine.data.vertices[i].co for i in indices],[],[tuple(mapping[i] for i in p.vertices) for p in faces]);data.update()
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj)
    for material in carbine.data.materials:data.materials.append(material)
    for new,old in zip(data.polygons,faces):new.material_index=old.material_index;new.use_smooth=old.use_smooth
    color=data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for cell in color.data:cell.color=(1,1,1,1)
    export_fbx(EXPORT/(name+'.fbx'),[obj]);return obj
carbine_body=copy_faces('SM_Carbine_C02Body',False);magazine=copy_faces('SM_Carbine_Magazine',True)
bpy.data.objects.remove(carbine,do_unlink=True);carbine=carbine_body
def anchor(name,bone):
    o=bpy.data.objects.new(name,None);bpy.context.collection.objects.link(o)
    c=o.constraints.new('COPY_TRANSFORMS');c.target=rig;c.subtarget=bone;c.target_space='WORLD';c.owner_space='WORLD'
    return o
weapon_bone=anchor('WeaponBone_Frame','weapon_r')
gun_frame=bpy.data.objects.new('GripCentred_WeaponFrame',None);bpy.context.collection.objects.link(gun_frame)
gun_frame.parent=weapon_bone;gun_frame.matrix_parent_inverse=Matrix.Identity(4)
gun_frame.matrix_basis=rig.data.bones['weapon_r'].matrix_local.to_3x3().inverted().to_4x4()
for obj in [shotgun,pump,carbine,magazine]:obj.parent=gun_frame;obj.matrix_parent_inverse=Matrix.Identity(4);obj.matrix_basis=Matrix.Identity(4)
hand_anchor=anchor('LoadingHand_Frame','hand_l');shell.parent=hand_anchor;shell.matrix_parent_inverse=Matrix.Identity(4)
shell.matrix_basis=rig.data.bones['hand_l'].matrix_local.to_3x3().inverted().to_4x4() @ Matrix.Translation((6.0,0,2.8))
held_magazine=magazine.copy();held_magazine.data=magazine.data;bpy.context.collection.objects.link(held_magazine);held_magazine.name='Held_magazine_preview'
held_magazine.parent=hand_anchor;held_magazine.matrix_parent_inverse=Matrix.Identity(4)
held_magazine.matrix_basis=rig.data.bones['hand_l'].matrix_local.to_3x3().inverted().to_4x4() @ Matrix.Translation((-10.5,0,0))
rig['pump_travel']=0.;rig['shell_visible']=0.;rig['weapon_kind']=1.;rig['carbine_mag_state']=0.
def property_driver(obj,path,expression,prop,index=None):
    f=obj.driver_add(path) if index is None else obj.driver_add(path,index)
    d=f.driver;d.type='SCRIPTED';d.expression=expression
    v=d.variables.new();v.name='value';v.type='SINGLE_PROP';v.targets[0].id=rig;v.targets[0].data_path='["'+prop+'"]'
property_driver(pump,'location','-9.0*value','pump_travel',0)
for obj in [shotgun,pump]:property_driver(obj,'hide_render','value < .5','weapon_kind')
property_driver(carbine,'hide_render','value > .5','weapon_kind')
property_driver(shell,'hide_render','value < .5','shell_visible')
property_driver(magazine,'hide_render','abs(value-1) > .1','carbine_mag_state')
property_driver(held_magazine,'hide_render','abs(value-2) > .1','carbine_mag_state')

def zero(arm):
    for bone in arm.pose.bones:
        bone.location=(0,0,0);bone.rotation_mode='QUATERNION';bone.rotation_quaternion=(1,0,0,0);bone.scale=(1,1,1)
def rotate(arm,name,axis,radians):
    bpy.context.view_layer.update();bone=arm.pose.bones[name];m=bone.matrix.copy();position=m.translation.copy()
    bone.matrix=Matrix.Translation(position)@Quaternion(Vector(axis),radians).to_matrix().to_4x4()@Matrix.Translation(-position)@m
    bpy.context.view_layer.update()
def move(arm,name,delta):
    bpy.context.view_layer.update();bone=arm.pose.bones[name];m=bone.matrix.copy();m.translation+=Vector(delta);bone.matrix=m
    bpy.context.view_layer.update()
def segment(arm,name,start,end):
    rest=arm.data.bones[name];q=(rest.tail_local-rest.head_local).normalized().rotation_difference((Vector(end)-Vector(start)).normalized())
    m=q.to_matrix().to_4x4()@rest.matrix_local.to_3x3().to_4x4();m.translation=Vector(start);arm.pose.bones[name].matrix=m
    bpy.context.view_layer.update()
def solve(arm,side,goal):
    bpy.context.view_layer.update();upper='upperarm_'+side;lower='lowerarm_'+side
    start=arm.pose.bones[upper].head.copy();goal=Vector(goal);delta=goal-start;direction=delta.normalized()
    a=arm.data.bones[upper].length;b=arm.data.bones[lower].length
    if delta.length>a+b-.01:goal=start+direction*(a+b-.01);delta=goal-start
    d=max(.1,delta.length);along=(a*a-b*b+d*d)/(2*d);height=math.sqrt(max(0,a*a-along*along))
    pole=Vector((6,-48 if side=='l' else 48,112));bend=(pole-start)-direction*(pole-start).dot(direction)
    joint=start+direction*along+bend.normalized()*height
    segment(arm,upper,start,joint);segment(arm,lower,joint,goal)
    return goal
def weapon_frame(arm):
    bpy.context.view_layer.update()
    return arm.pose.bones['weapon_r'].matrix @ arm.data.bones['weapon_r'].matrix_local.to_3x3().inverted().to_4x4()
def support(arm,offset=(18,0,6)):
    frame=weapon_frame(arm);goal=solve(arm,'l',frame@Vector(offset))
    m=frame.to_3x3().to_4x4()@arm.data.bones['hand_l'].matrix_local.to_3x3().to_4x4();m.translation=goal
    arm.pose.bones['hand_l'].matrix=m;bpy.context.view_layer.update()
def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
def keyed(time,points):
    if time<=points[0][0]:return points[0][1]
    if time>=points[-1][0]:return points[-1][1]
    for (a,va),(b,vb) in zip(points,points[1:]):
        if a<=time<=b:return va+(vb-va)*smooth((time-a)/(b-a))
def reload_posture(amount):
    rotate(rig,'spine_02',(0,0,1),math.radians(4*amount))
    rotate(rig,'upperarm_r',(0,1,0),math.radians(10*amount))
    rotate(rig,'hand_r',(1,0,0),math.radians(-27*amount))
def animate(name,duration,kind):
    action=bpy.data.actions.new('A_Response_'+name);action.use_fake_user=True;rig.animation_data_create();rig.animation_data.action=action
    scene.frame_start=1;scene.frame_end=round(duration*FPS)+1
    samples=[]
    for index in range(round(duration*FPS)+1):
        time=index/FPS;scene.frame_set(index+1);zero(rig);rig['pump_travel']=0.;rig['shell_visible']=0.;rig['weapon_kind']=0. if kind=='carbine_reload' else 1.;rig['carbine_mag_state']=0.
        if kind=='ready':
            rotate(rig,'spine_02',(0,1,0),math.radians(.3*math.sin(time/duration*math.tau)))
            support(rig)
        elif kind=='fire':
            kick=keyed(time,[(0,0),(.028,1),(.08,.45),(.16,.10),(.22,0)])
            rotate(rig,'spine_02',(0,1,0),math.radians(-3.0*kick))
            rotate(rig,'upperarm_r',(0,1,0),math.radians(-4.0*kick));support(rig)
        elif kind=='pump':
            travel=keyed(time,[(0,0),(.21,1),(.44,0),(.56,0)]);rig['pump_travel']=float(travel)
            support(rig,(18-9*travel,0,6))
        elif kind=='reload_start':
            amount=smooth(time/duration);reload_posture(amount)
            # First bring the support hand clear of the fore-end; end matches shell-loop start.
            support(rig,Vector((18,0,6)).lerp(Vector((13,-8,-1)),amount))
        elif kind=='reload_shell':
            reload_posture(1)
            frame=weapon_frame(rig)
            if time<.24:
                u=smooth(time/.24);goal=(frame@Vector((13,-8,-1))).lerp(Vector((13,-9,108)),u)
            elif time<.60:
                u=smooth((time-.24)/.36);goal=Vector((13,-9,108)).lerp(frame@Vector((2,0,.6)),u)
            else:
                u=smooth((time-.60)/.30);goal=(frame@Vector((2,0,.6))).lerp(frame@Vector((13,-8,-1)),u)
            goal=solve(rig,'l',goal)
            orientation=frame.to_3x3().to_4x4()@rig.data.bones['hand_l'].matrix_local.to_3x3().to_4x4();orientation.translation=goal;rig.pose.bones['hand_l'].matrix=orientation
            rig['shell_visible']=1. if .12<=time<.60 else 0.
        elif kind=='reload_end':
            amount=1-smooth(time/duration);reload_posture(amount)
            support(rig,Vector((18,0,6)).lerp(Vector((13,-8,-1)),amount))
        elif kind=='equip':
            lower=max(0,math.sin(math.pi*time/duration))**1.4
            rotate(rig,'upperarm_r',(0,1,0),math.radians(24*lower));rotate(rig,'spine_02',(0,0,1),math.radians(5*lower));support(rig)
            rig['weapon_kind']=0. if time<.18 else 1.
            rig['carbine_mag_state']=1. if time<.18 else 0.
        elif kind=='carbine_reload':
            amount=keyed(time,[(0,0),(.22,1),(1.80,1),(2.1,0)])
            rotate(rig,'spine_02',(0,0,1),math.radians(4*amount));rotate(rig,'upperarm_r',(0,1,0),math.radians(5*amount))
            frame=weapon_frame(rig)
            offsets=[(0,Vector((18,0,6))),(.28,Vector((10.5,0,0))),(.40,Vector((10.5,0,-3))),(.76,Vector((-10,-10,-24))),
                (1.08,Vector((10.5,0,-9))),(1.20,Vector((10.5,0,0))),(1.46,Vector((13.5,0,1))),(1.68,Vector((4.5,4,13))),
                (1.74,Vector((1.5,4,13))),(1.88,Vector((8,0,9))),(2.10,Vector((18,0,6)))]
            support(rig,keyed(time,offsets))
            rig['carbine_mag_state']=2. if .40<=time<1.20 else 1.
        bpy.context.view_layer.update()
        for bone in rig.pose.bones:
            bone.keyframe_insert('location',frame=index+1,group=bone.name);bone.keyframe_insert('rotation_quaternion',frame=index+1,group=bone.name);bone.keyframe_insert('scale',frame=index+1,group=bone.name)
        for prop in ['pump_travel','shell_visible','weapon_kind','carbine_mag_state']:rig.keyframe_insert('["'+prop+'"]',frame=index+1)
        if index in [0,round(duration*FPS/2),round(duration*FPS)]:samples.append({'time':time,'wrist':list(rig.pose.bones['hand_l'].head),'weapon_origin':list(rig.pose.bones['weapon_r'].head),'pump_travel':rig['pump_travel']})
    scene.frame_set(1);export_fbx(EXPORT/(action.name+'.fbx'),[rig],True)
    return action,samples

definitions=[('ShotgunReady',2.4,'ready'),('ShotgunFire',.22,'fire'),('ShotgunPump',.56,'pump'),
    ('ShotgunReloadStart',.35,'reload_start'),('ShotgunReloadShell',.90,'reload_shell'),('ShotgunReloadEnd',.32,'reload_end'),
    ('Equip',.36,'equip'),('CarbineReload',2.1,'carbine_reload')]
actions={};checks={}
for name,duration,kind in definitions:actions[name],checks[name]=animate(name,duration,kind)

# Store a useful source setup with the accepted player, both guns and automatically
# driven fore-end/shell preview, all original C01 reference actions retained.
rig.animation_data.action=actions['ShotgunPump'];scene.frame_start=1;scene.frame_end=57;scene.frame_set(22)
world=bpy.data.worlds.new('Candidate02_weapon_workshop');world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.1,.12,.14,1);world.node_tree.nodes['Background'].inputs[1].default_value=.6;scene.world=world
def light(name,pos,power,size):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(obj);obj.location=pos
    obj.rotation_euler=(Vector((10,0,120))-obj.location).to_track_quat('-Z','Y').to_euler()
light('Key',(160,-140,270),400000,130);light('Fill',(40,170,190),260000,110)
camera_data=bpy.data.cameras.new('C02_source_review');camera=bpy.data.objects.new('C02_source_review',camera_data);bpy.context.collection.objects.link(camera)
camera.location=(280,-310,235);camera.rotation_euler=(Vector((22,0,113))-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.type='ORTHO';camera_data.ortho_scale=190;camera_data.clip_end=2000;scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1300;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX';scene.render.image_settings.file_format='PNG'
bpy.ops.wm.save_as_mainfile(filepath=str(SRC/'ResponseWeaponActions.blend'))
scene.render.filepath=str(SRC/'Blender_ShotgunPump.png');bpy.ops.render.render(write_still=True)
rig.animation_data.action=actions['ShotgunReloadShell'];scene.frame_set(60)
scene.render.filepath=str(SRC/'Blender_ShotgunShellInsert.png');bpy.ops.render.render(write_still=True)
rig.animation_data.action=actions['CarbineReload'];scene.frame_set(121)
scene.render.filepath=str(SRC/'Blender_CarbineMagazineSeat.png');bpy.ops.render.render(write_still=True)

# New stronger reaction on the existing infected rig only; no mesh or old FBX changes.
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'ArtSource'/'Characters'/'Infected.blend'))
scene=bpy.context.scene;preserve_action_durations(scene.render.fps);scene.render.fps=FPS;enemy=bpy.data.objects['Rig_Infected'];enemy.location=(0,0,0)
reaction=bpy.data.actions.new('A_Infected_HeavyHit');reaction.use_fake_user=True;enemy.animation_data_create();enemy.animation_data.action=reaction
scene.frame_start=1;scene.frame_end=53
for frame in range(53):
    scene.frame_set(frame+1);zero(enemy);t=frame/FPS;e=keyed(t,[(0,0),(.12,1),(.23,.72),(.52,0)])
    rotate(enemy,'spine_01',(0,1,0),math.radians(12-19*e));rotate(enemy,'spine_02',(0,1,0),math.radians(5-8*e))
    rotate(enemy,'neck',(0,1,0),math.radians(-11-7*e));rotate(enemy,'spine_02',(0,0,1),math.radians(-7*e))
    rotate(enemy,'upperarm_r',(0,1,0),math.radians(-12*e));rotate(enemy,'upperarm_l',(0,1,0),math.radians(7*e))
    for bone in enemy.pose.bones:
        bone.keyframe_insert('location',frame=frame+1,group=bone.name);bone.keyframe_insert('rotation_quaternion',frame=frame+1,group=bone.name);bone.keyframe_insert('scale',frame=frame+1,group=bone.name)
scene.frame_set(1);export_fbx(EXPORT/'A_Infected_HeavyHit.fbx',[enemy],True)
bpy.ops.wm.save_as_mainfile(filepath=str(SRC/'InfectedHeavyReaction.blend'))

audio_inventory=create_audio()
inventory={'candidate':'02','provenance':'Original Project ONE authoring; preserves accepted C01 rigs/characters and splits the accepted carbine into body/magazine without changing its surface geometry; no third-party assets',
 'source':'ArtSource/Weapons/Candidate02/ResponseWeaponActions.blend','fps':FPS,'skeleton':'/Game/ONE/Characters/SK_Response_Skeleton',
 'static_meshes':['SM_PumpShotgun','SM_PumpShotgun_ForeEnd','SM_ShotgunShell','SM_Carbine_C02Body','SM_Carbine_Magazine'],
 'materials':{name:palette[name] for name in ['M_ShotgunPolymer','M_ShellHull','M_Brass']},
 'animations':{('A_Response_'+name):{'duration':duration,'use':kind,'samples':checks[name]} for name,duration,kind in definitions},
 'infected_reaction':{'name':'A_Infected_HeavyHit','duration':.52,'source':'ArtSource/Weapons/Candidate02/InfectedHeavyReaction.blend'},
 'runtime':{'weapon_origin':'Grip-centred, preserve existing weapon_r bind-inverse attachment','muzzle_ue_cm':[64.5,0,14],
 'fore_end_center_ue_cm':[18,0,6],'fore_end_travel_cm':[-9,0,0],'pump_curve_seconds':[[0,0],[.21,1],[.44,0],[.56,0]],
 'pump_eject_seconds':.18,'eject_origin_ue_cm':[5,-4.5,13.5],'shell_axis':'Mesh cylinder local +X, mesh origin centre',
 'shell_insert_seconds':.60,'shell_visible_seconds':[.12,.60],'shell_hand_offset_gun_axes':[6,0,2.8],
 'carbine_magazine_mesh_center_gun_axes':[13.5,0,-2],'held_magazine_translation_gun_axes':[-10.5,0,0],
 'equip_swap_seconds':.18,'carbine_mag_out':.40,'carbine_mag_in':1.20,'carbine_bolt':1.74},
 'audio':audio_inventory,'limitations':['Source PNGs are Blender QA, not gameplay evidence','Original synthesized sound-design candidates require gameplay audition; no claim of recorded-real-weapon fidelity','Existing mirrored anatomical bone labels preserved intentionally']}
(SRC/'inventory.json').write_text(json.dumps(inventory,indent=2))
print('CANDIDATE02_WEAPON_ASSETS_COMPLETE',len(audio_inventory),'sounds')
