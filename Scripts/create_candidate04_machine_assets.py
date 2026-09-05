"""Author the original Candidate04 containment chest and weapon processor.

Blender 5.1: --background --disable-autoexec --python-exit-code 1 --python this_script
Editable centimetre geometry, materials and mechanical pivots. No imported packs.
This is a separate background process; it never operates on an existing GUI file.
Source coordinates use +X toward the operator. Legacy FBX import preserves X/Z
and reflects Y; the manifest supplies the corresponding Unreal part origins.
"""
from pathlib import Path
import bpy
from mathutils import Vector
import argparse
import hashlib
import json
import math
import random
import sys

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/Machines/Candidate04'
EXPORT=OUT/'Exports'
OUT.mkdir(parents=True,exist_ok=True);EXPORT.mkdir(exist_ok=True)
args=argparse.ArgumentParser();args.add_argument('--no-render',action='store_true')
opt=args.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
scene.render.engine='CYCLES';scene.cycles.samples=36
scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('Neutral studio');scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.20,.24,.29,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.55

PALETTE={
 'M_C04_Carbon':{'color':[.029,.044,.055,1],'roughness':.69,'metallic':.65},
 'M_C04_Armor':{'color':[.058,.095,.116,1],'roughness':.65,'metallic':.52},
 'M_C04_Ceramic':{'color':[.48,.52,.48,1],'roughness':.65,'metallic':.15},
 'M_C04_Teal':{'color':[.025,.21,.22,1],'roughness':.57,'metallic':.55},
 'M_C04_Steel':{'color':[.22,.27,.29,1],'roughness':.43,'metallic':.86},
 'M_C04_Edge':{'color':[.42,.48,.50,1],'roughness':.42,'metallic':.83},
 'M_C04_Copper':{'color':[.42,.21,.065,1],'roughness':.51,'metallic':.78},
 'M_C04_Rubber':{'color':[.014,.019,.022,1],'roughness':.92,'metallic':0},
 'M_C04_Amber':{'color':[.87,.31,.025,1],'roughness':.65,'metallic':.26},
 'M_C04_Stencil':{'color':[.67,.71,.62,1],'roughness':.85,'metallic':0},
 'M_C04_Lamp':{'color':[.28,.70,.85,1],'roughness':.32,'metallic':.10,'emission':3.5},
 'M_C04_Violet':{'color':[.43,.12,.88,1],'roughness':.32,'metallic':.10,'emission':3.5},
 'M_C04_Screen':{'color':[.008,.027,.035,1],'roughness':.36,'metallic':.25},
}
MATS={}
for name,row in PALETTE.items():
    m=bpy.data.materials.new(name);m.diffuse_color=row['color'];m.use_nodes=True
    nodes=m.node_tree.nodes;bs=nodes.get('Principled BSDF')
    col=nodes.new('ShaderNodeVertexColor');col.layer_name='Color'
    mul=nodes.new('ShaderNodeMixRGB');mul.blend_type='MULTIPLY';mul.inputs[0].default_value=1
    mul.inputs[1].default_value=row['color'];m.node_tree.links.new(col.outputs['Color'],mul.inputs[2])
    m.node_tree.links.new(mul.outputs['Color'],bs.inputs['Base Color'])
    bs.inputs['Roughness'].default_value=row['roughness'];bs.inputs['Metallic'].default_value=row['metallic']
    if row.get('emission'):
        bs.inputs['Emission Color'].default_value=row['color'];bs.inputs['Emission Strength'].default_value=row['emission']
    MATS[name]=m

parts=[];assets={};asset_rows={};rng=random.Random(440407)
def finish(o,name,mat,bevel=.6):
    o.name=name;o.data.materials.append(MATS[mat]);bpy.context.view_layer.objects.active=o
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Cast and machined edge radii','BEVEL');mod.width=bevel;mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    attr=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for polygon in o.data.polygons:
        polygon.use_smooth=False
        face_gain=rng.uniform(.90,1.)
        for i in polygon.loop_indices:attr.data[i].color=(face_gain,face_gain,face_gain,1)
    parts.append(o);return o
def box(name,loc,size,mat='M_C04_Armor',bevel=.6,rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size;o.rotation_euler=rot
    return finish(o,name,mat,bevel)
def cyl(name,loc,radius,depth,mat='M_C04_Steel',axis='Z',vertices=20,bevel=.25):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc)
    o=bpy.context.object
    if axis=='X':o.rotation_euler.y=math.pi/2
    elif axis=='Y':o.rotation_euler.x=math.pi/2
    return finish(o,name,mat,bevel)
def beam(name,a,b,radius,mat='M_C04_Steel',vertices=12):
    a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,radius,(b-a).length,mat,vertices=vertices)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def ring(name,loc,major,minor,mat='M_C04_Copper',axis='Z'):
    bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=40,minor_segments=8,location=loc)
    o=bpy.context.object
    if axis=='X':o.rotation_euler.y=math.pi/2
    if axis=='Y':o.rotation_euler.x=math.pi/2
    return finish(o,name,mat,0)
def text(name,body,loc,size,mat='M_C04_Stencil',front=True):
    cu=bpy.data.curves.new(name,'FONT');cu.body=body;cu.align_x='CENTER';cu.size=size;cu.extrude=.035;cu.bevel_depth=.01
    o=bpy.data.objects.new(name,cu);bpy.context.collection.objects.link(o);o.location=loc
    # Text +Z normal rotates toward +X; local text baseline becomes +Y.
    if front:o.rotation_euler=(math.pi/2,0,math.pi/2)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    bpy.ops.object.convert(target='MESH');return finish(bpy.context.object,name,mat,0)
def panel_fasteners(x,ys,zs,mat='M_C04_Edge'):
    for y in ys:
        for z in zs:
            cyl('Recessed fastener seat',(x-.12,y,z),1.25,.35,'M_C04_Carbon','X',12,.10)
            cyl('Captive six-point bolt',(x,y,z),.70,.46,mat,'X',6,.10)
def pipe(name,points,radius,mat='M_C04_Copper'):
    for a,b in zip(points,points[1:]):beam(name,a,b,radius,mat)
    for p in points[1:-1]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=6,radius=radius,location=p)
        finish(bpy.context.object,name+' elbow',mat,0)
def commit(name,pivot=(0,0,0),family='box',motion='fixed'):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    original=o.location.copy();o.location=(0,0,0)
    path=EXPORT/(name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},
        use_mesh_modifiers=True,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,
        mesh_smooth_type='FACE',bake_anim=False,use_custom_props=False,add_leaf_bones=False)
    o.location=original;o.data.calc_loop_triangles()
    coords=[v.co for v in o.data.vertices]
    lo=[min(v[i] for v in coords) for i in range(3)];hi=[max(v[i] for v in coords) for i in range(3)]
    asset_rows[name]={'source':path.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/Machines/Candidate04/'+name,
      'family':family,'motion':motion,'ue_origin_cm':[pivot[0],-pivot[1],pivot[2]],
      'source_bounds_local_cm':{'min':lo,'max':hi},'ue_bounds_local_cm':{'min':[lo[0],-hi[1],lo[2]],'max':[hi[0],-lo[1],hi[2]]},
      'vertices':len(o.data.vertices),'triangles':len(o.data.loop_triangles),
      'materials':[m.name for m in o.data.materials],'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    assets[name]=o;parts=[];o.hide_set(True);return o

# CONTAINMENT CHEST: freestanding nested casing, hollow well, real hinge/locks.
for y in (-77,77):
    box('Damped transport runner',(-2,y,6),(111,18,12),'M_C04_Rubber',2)
    box('Runner steel shoe',(-2,y,10),(114,16,5),'M_C04_Steel',1)
box('Reinforced underpan',(-2,0,18),(108,184,20),'M_C04_Carbon',4)
box('Interior well floor',(-2,0,40),(96,166,18),'M_C04_Armor',2)
for x in (-49,47):
    box('Double wall pressure case',(x,0,61),(9,184,68),'M_C04_Armor',2.8)
    box('Inlaid front skin',(x+(5 if x>0 else -5),0,59),(3,145,48),'M_C04_Carbon',1.5)
for y in (-87,87):
    box('Side pressure casing',(-1,y,63),(99,10,68),'M_C04_Carbon',2.8)
    box('Side armor cheek',(-2,y+(-5 if y<0 else 5),63),(77,3.5,43),'M_C04_Teal',1.5)
    for x in (-34,31):box('Corner impact upright',(x,y,59),(14,14,73),'M_C04_Steel',1.9)
    # Transport handle has a genuinely open center.
    for x in (-15,15):beam('Handle standoff',(x,y,64),(x,y*1.13,64),2.2)
    beam('Folded lifting handle',(-15,y*1.13,64),(15,y*1.13,64),2.4,'M_C04_Rubber')
for y in (-73,73):
    box('Top well rim',(-1,y,95),(104,8,5),'M_C04_Edge',.8)
    box('Compressed lid gasket',(-1,y-5*(1 if y>0 else -1),96),(93,2,2),'M_C04_Rubber',.4)
for x in (-46,46):box('End well rim',(x,0,95),(8,148,5),'M_C04_Edge',.7)
for y in (-62,62):
    cyl('Rear hinge knuckle',(-48,y,96),5,22,'M_C04_Steel','Y',24)
    cyl('Hinge pin cap',(-48,y+12,96),2.1,2,'M_C04_Copper','Y',12)
    box('Lock socket',(55,y,80),(7,16,21),'M_C04_Carbon',1)
    box('Lock socket amber guide',(59,y,77),(1,13,3),'M_C04_Amber',.25)
for y in (-64,64):
    box('Internal induction track',(-2,y,61),(73,10,7),'M_C04_Steel',1.2)
    box('Track luminous ceramic',(-2,y,66),(65,3,2),'M_C04_Violet',.3)
    for x in (-31,-16,-1,14,29):box('Coil cooling fin',(x,y,71),(3,12,13),'M_C04_Copper',.5)
for x in (-33,33):
    beam('Internal cross brace',(x,-53,48),(x,53,48),2,'M_C04_Steel')
for y in (-50,-25,0,25,50):
    box('Interior baffle',(-39,y,70),(3,17,30),'M_C04_Rubber',.6)
    box('Well back status strip',(-36.7,y,84),(1,12,1.2),'M_C04_Lamp',.15)
panel_fasteners(57,(-67,67),(43,75))
box('Front identity plate',(57,0,67),(1,106,20),'M_C04_Teal',.6)
text('Embossed chest identity','MYSTERY BOX',(57.8,0,68),7.3)
text('Containment serial','MB - 04   /   CONTAINMENT',(57.8,0,60),2.6)
for y in range(-54,55,12):
    box('Inset front lower vents',(56,y,39),(2,6,2),'M_C04_Rubber',.4)
for y in (-70,70):
    box('Front operating lamp',(59,y,90),(1.5,6,2),'M_C04_Violet',.4)
pipe('External protected conduit',[(-54,-65,35),(-58,-65,26),(-58,60,26),(-54,60,51)],1.2,'M_C04_Copper')
commit('SM_C04_BoxBody')

# Lid: panel layers, underside bracing, warning rails and attached hinge ears.
box('Lid armored shell',(1,0,101),(108,186,11),'M_C04_Carbon',3.8)
box('Lid inset upper skin',(3,0,107),(83,152,2),'M_C04_Armor',1.3)
for y in (-65,65):
    box('Lid raised structural spine',(2,y,109),(91,9,5),'M_C04_Steel',1.1)
    box('Lid wear strip',(4,y,112),(68,3,1),'M_C04_Edge',.3)
for y in (-80,80):box('Outer amber warning strip',(6,y,107),(76,2.5,1.5),'M_C04_Amber',.4)
for y in (-45,45):
    box('Interior diagonal reinforcement',(0,y,94),(90,5,3),'M_C04_Steel',.6,rot=(0,0,math.radians(12)*(1 if y>0 else -1)))
    box('Lid interior soft emitter',(0,y,92),(69,2,1),'M_C04_Violet',.25)
for y in (-63,63):box('Lid hinge ear',(-46,y,99),(15,17,9),'M_C04_Armor',1.7)
text('Lid top designation','ONE / B-07',(3,0,109),10,front=False)
commit('SM_C04_BoxLid',(-48,0,96),motion='Pitch 0..108 degrees')
box('Sliding lock tongue',(0,0,0),(7,13,24),'M_C04_Steel',1)
box('Lock reinforced face',(4,0,1),(3,16,13),'M_C04_Copper',.7)
cyl('Lock visible spindle',(6,0,2),2.9,1.2,'M_C04_Carbon','X',12)
box('Lock status',(7,0,-5),(1,8,1.4),'M_C04_Lamp',.2)
commit('SM_C04_BoxLock',motion='Two instances at X59 Y+/-62 Z89; slide 13cm outwards')
cyl('Induction rotor bed',(0,0,0),28,6,'M_C04_Carbon',vertices=40)
ring('Induction main coil',(0,0,4),23,2.5,'M_C04_Copper')
ring('Containment luminous ring',(0,0,6),19,1.2,'M_C04_Violet')
for i in range(8):
    a=math.tau*i/8
    box('Rotor segmented ferrite',(23*math.cos(a),23*math.sin(a),6),(8,5,5),'M_C04_Steel',.5,rot=(0,0,a))
cyl('Rotor center', (0,0,5),12,4,'M_C04_Screen',vertices=32)
ring('Inner emitter',(0,0,8),8,.65,'M_C04_Violet')
commit('SM_C04_BoxRotor',(0,0,0),motion='Instance at(0,0,52), slow Z rotation')

# PROCESSOR: broad plinth, open chamber, substantial return platform and service columns.
for y in (-83,83):box('Processor isolator',(0,y,6),(132,27,12),'M_C04_Rubber',2)
box('Stepped cast foundation',(-3,0,20),(141,207,27),'M_C04_Carbon',5)
box('Lower processor enclosure',(-18,0,57),(110,183,58),'M_C04_Ceramic',4)
for y in (-91,91):
    box('Structural copper cheek',(-14,y,66),(83,7,71),'M_C04_Copper',2)
    box('Intake upright',(13,y,133),(46,24,126),'M_C04_Ceramic',3)
    box('Inner upright dark rail',(35,y*.83,138),(10,8,114),'M_C04_Carbon',1.4)
    box('Column steel wear edge',(39,y,141),(4,17,101),'M_C04_Steel',.6)
    for z in (109,131,153,175):
        box('Column inset louver',(42,y,z),(3,12,3),'M_C04_Rubber',.5)
    box('Column ready indicator',(44,y,188),(2,14,4),'M_C04_Lamp',.5)
    panel_fasteners(41,(y-7,y+7),(92,180))
box('Chamber crown',(-9,0,194),(102,186,20),'M_C04_Ceramic',4)
box('Crown inner lining',(19,0,181),(36,151,8),'M_C04_Carbon',1.5)
box('Top service spine',(-35,0,207),(46,139,10),'M_C04_Steel',2)
for y in range(-57,58,13):box('Crown heat sink',(-35,y,215),(42,3,10),'M_C04_Carbon',.7)
box('Processor name plate',(44,0,188),(2,131,19),'M_C04_Teal',.7)
text('Processor name','PACK-A-PUNCH',(45.5,0,190),7)
text('Processor designation','WEAPON PROCESSOR / PP-04',(45.5,0,183),2.5)
box('Chamber rear service wall',(-67,0,131),(12,180,117),'M_C04_Carbon',3)
for y in (-56,56):
    box('Rear power cartridge',(-55,y,137),(10,29,70),'M_C04_Teal',1.5)
    for z in range(113,166,10):box('Cartridge copper buss',(-48,y,z),(3,23,3),'M_C04_Copper',.4)
    pipe('Insulated current lead',[(-48,y,172),(-32,y,176),(-14,y,166)],2.3,'M_C04_Rubber')
box('Processing well',(-15,0,89),(100,148,11),'M_C04_Steel',1.8)
for y in (-63,63):
    box('Feed rail support',(45,y,93),(153,12,15),'M_C04_Carbon',1.8)
    cyl('Linear guide',(48,y,104),2.6,154,'M_C04_Edge','X',20)
    box('Rail wiper',(114,y,103),(6,11,9),'M_C04_Rubber',.5)
box('Return tray outer frame',(95,0,97),(48,148,10),'M_C04_Steel',1.7)
box('Return tray recessed bed',(95,0,103),(41,133,2),'M_C04_Rubber',.6)
for y in (-75,75):box('Output safety edge',(95,y,105),(48,4,8),'M_C04_Amber',.7)
for y in range(-58,59,14):box('Tray perforation',(96,y,104),(32,5,1),'M_C04_Carbon',.25)
box('Front electrical service panel',(40,0,57),(3,115,39),'M_C04_Teal',1)
panel_fasteners(42,(-52,52),(42,72))
text('Powered from outset','POWERED  /  5000',(43,0,63),5.0)
text('Intake instruction','PLACE WEAPON ON CRADLE',(43,0,50),3.1)
for y in (-34,-17,0,17,34):
    cyl('Panel breaker',(43,y,37),2.3,2,'M_C04_Carbon','X',16)
    cyl('Breaker lamp',(44.2,y,37),.8,.6,'M_C04_Lamp','X',12,.1)
for y in (-100,100):
    cyl('Coolant tank',(-43,y,137),8,68,'M_C04_Steel',vertices=24)
    for z in (105,129,154,170):ring('Coolant tank clamp',(-43,y,z),8.3,.8,'M_C04_Copper')
    pipe('Coolant delivery',[(-43,y,172),(-14,y,180),(5,y*.65,180)],1.3,'M_C04_Copper')
    pipe('Coolant return',[(-43,y,103),(-35,y,78),(20,y*.78,78)],1.5,'M_C04_Rubber')
commit('SM_C04_UpgradeBody',family='upgrade')

# Moving cradle and paired independently closing clamp fingers.
box('Moving cradle',(0,0,-5),(36,123,5),'M_C04_Steel',1.4)
for y in (-47,47):
    box('V saddle',(0,y,0),(18,13,9),'M_C04_Rubber',1.4)
    box('Saddle copper bearing',(0,y,-4),(29,17,3),'M_C04_Copper',.5)
for x in (-14,14):box('Cradle edge',(x,0,-2),(3,116,5),'M_C04_Edge',.6)
commit('SM_C04_UpgradeCradle',family='upgrade',motion='Translate X94..-5 Z107..121')
box('Clamp base',(0,0,0),(13,13,7),'M_C04_Carbon',1)
box('Clamp vertical actuator',(0,0,9),(10,9,18),'M_C04_Steel',1)
box('Clamp inward jaw',(-7,0,17),(22,10,7),'M_C04_Copper',.8)
box('Clamp soft contact',(-17,0,16),(3,11,6),'M_C04_Rubber',.6)
cyl('Clamp pivot',(3,0,14),3,12,'M_C04_Edge','Y',16)
commit('SM_C04_UpgradeClamp',family='upgrade',motion='Two local X mirrored fingers, move X38..22 relative to cradle')
ring('Processing field ring',(0,0,0),46,5,'M_C04_Carbon','Y')
ring('Copper field winding',(0,-3,0),42,2.4,'M_C04_Copper','Y')
ring('Illuminated field ceramic',(0,3,0),39.5,1.7,'M_C04_Lamp','Y')
for i in range(10):
    a=math.tau*i/10
    box('Field pole',(45*math.cos(a),0,45*math.sin(a)),(12,14,8),'M_C04_Steel',1,rot=(0,-a,0))
commit('SM_C04_UpgradeRing',family='upgrade',motion='Two chamber Y+/-38 instances rotating slowly around Y')
box('Sliding chamber shield',(0,0,0),(6,67,81),'M_C04_Armor',2.5)
box('Shield pale insert',(4,0,0),(3,53,63),'M_C04_Ceramic',1.4)
for z in (-23,0,23):box('Shield inset slit',(6,0,z),(1,48,2.2),'M_C04_Rubber',.4)
box('Shield diagnostic strip',(6,-21,0),(2,3,56),'M_C04_Lamp',.4)
commit('SM_C04_UpgradeShield',family='upgrade',motion='Rear side shields Y+/-74 slide inward22; center remains readable')

# Simple local-axis strut components can be driven between physical lid anchors.
cyl('Hydraulic sleeve',(0,0,0),2,1,'M_C04_Carbon',vertices=16,bevel=.1)
commit('SM_C04_HydraulicSleeve',family='shared',motion='Unit Z length; scale only along Z between physical attachment points')
cyl('Hydraulic piston',(0,0,0),1.1,1,'M_C04_Edge',vertices=16,bevel=.05)
commit('SM_C04_HydraulicRod',family='shared',motion='Unit Z length; telescoping physical lid support')

# An assembled source scene stores useful animation rigs as parenting/transforms.
roots={}
for family in ('box','upgrade'):
    root=bpy.data.objects.new('Assembly_'+family,None);bpy.context.collection.objects.link(root);roots[family]=root
for name,o in assets.items():
    o.hide_set(False)
    if asset_rows[name]['family']=='shared':o.hide_render=True;o.hide_set(True);continue
    o.parent=roots[asset_rows[name]['family']]
assets['SM_C04_BoxRotor'].location=(0,0,52)
assets['SM_C04_BoxLock'].location=(59,-62,89)
lock2=assets['SM_C04_BoxLock'].copy();lock2.data=assets['SM_C04_BoxLock'].data.copy();bpy.context.collection.objects.link(lock2);lock2.location=(59,62,89)
for name in ('SM_C04_UpgradeCradle','SM_C04_UpgradeClamp'):assets[name].location=(94,0,107)
assets['SM_C04_UpgradeClamp'].location=(132,-24,107)
clamp2=assets['SM_C04_UpgradeClamp'].copy();clamp2.data=assets['SM_C04_UpgradeClamp'].data.copy();bpy.context.collection.objects.link(clamp2);clamp2.location=(56,24,107);clamp2.rotation_euler.z=math.pi
assets['SM_C04_UpgradeRing'].location=(-15,-37,130)
ring2=assets['SM_C04_UpgradeRing'].copy();ring2.data=assets['SM_C04_UpgradeRing'].data.copy();bpy.context.collection.objects.link(ring2);ring2.location=(-15,37,130)
assets['SM_C04_UpgradeShield'].location=(-40,-70,139)
shield2=assets['SM_C04_UpgradeShield'].copy();shield2.data=assets['SM_C04_UpgradeShield'].data.copy();bpy.context.collection.objects.link(shield2);shield2.location=(-40,70,139)
lid=assets['SM_C04_BoxLid']
for f,ang in ((1,0),(25,0),(55,-108),(180,-108),(215,0)):
    lid.rotation_euler.y=math.radians(ang);lid.keyframe_insert(data_path='rotation_euler',frame=f)
for o,sign in ((assets['SM_C04_BoxLock'],-1),(lock2,1)):
    for f,d in ((1,0),(12,0),(25,13),(195,13),(215,0)):
        o.location.y=sign*(62+d);o.keyframe_insert(data_path='location',frame=f)
for f,x,z in ((1,94,107),(36,-5,121),(231,-5,121),(270,94,107)):
    assets['SM_C04_UpgradeCradle'].location=(x,0,z);assets['SM_C04_UpgradeCradle'].keyframe_insert(data_path='location',frame=f)
    for o,sign in ((assets['SM_C04_UpgradeClamp'],1),(clamp2,-1)):
        spread=38 if f in (1,270) else 22
        o.location=(x+sign*spread,-sign*24,z);o.keyframe_insert(data_path='location',frame=f)
for o in (assets['SM_C04_UpgradeRing'],ring2):
    for f,angle in ((1,0),(270,math.tau)):
        o.rotation_euler.y=angle;o.keyframe_insert(data_path='rotation_euler',frame=f)
roots['box'].location=(0,-180,0);roots['upgrade'].location=(0,170,0)
scene.frame_start=1;scene.frame_end=270;scene.render.fps=30;scene.frame_set(75)

# Original studio renders; camera/light helpers are excluded from each FBX.
floor_mat=bpy.data.materials.new('Studio floor');floor_mat.diffuse_color=(.075,.09,.105,1)
bpy.ops.mesh.primitive_plane_add(size=2200,location=(0,0,-1));floor=bpy.context.object;floor.name='Preview studio ground';floor.data.materials.append(floor_mat)
for name,pos,power,size,color in (
 ('Broad soft key',(420,-260,620),5500,450,(.79,.9,1)),
 ('Warm rim',(-220,360,430),5000,350,(1,.76,.46)),
 ('Front fill',(500,410,320),3000,340,(.75,.92,1))):
    data=bpy.data.lights.new(name,'AREA');data.energy=power*1000;data.shape='DISK';data.size=size;data.color=color
    o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o);o.location=pos;o.rotation_euler=(Vector((0,0,100))-o.location).to_track_quat('-Z','Y').to_euler()
camera_data=bpy.data.cameras.new('Machine review camera');camera=bpy.data.objects.new('Machine review camera',camera_data);bpy.context.collection.objects.link(camera);scene.camera=camera
camera_data.type='ORTHO';camera_data.lens=48;camera_data.clip_end=5000
def camera_at(position,target,scale):
    camera.location=position;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.ortho_scale=scale
camera_at((620,-650,520),(0,0,104),700)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ProjectONE_Machines_C04.blend'))

manifest={'candidate':'04','units':'centimetres','authoring':'Original procedural Project ONE machine geometry; no third-party asset inputs.',
 'coordinates':{'ue_forward':'+X toward operator','ue_width':'+Y','up':'+Z','fbx_import':{'axis_forward':'-Y','axis_up':'Z','convert_scene':True,'force_front_x_axis':False},'all_preview_weapons_scale':1},
 'source':'ArtSource/Machines/Candidate04/ProjectONE_Machines_C04.blend','static_meshes':asset_rows,'materials':PALETTE,
 'machines':{'box':{'body':'SM_C04_BoxBody','lid':'SM_C04_BoxLid','lid_pivot_cm':[-48,0,96],'lid_open_pitch_degrees':108,'lock_origins_cm':[[59,-62,89],[59,62,89]],'preview_mount_cm':[0,0,139],'collision_center_cm':[3,0,52],'collision_half_extent_cm':[65,102,52],'suggested_world_cm':[-650,-530,2],'suggested_yaw':0},
 'upgrade':{'body':'SM_C04_UpgradeBody','intake_mount_cm':[94,0,107],'processing_mount_cm':[-5,0,121],'output_mount_cm':[94,0,107],'preview_yaw_degrees':90,'preview_centering':'Subtract assembled weapon bounds center.X from machine mount.Y, without scaling.','active_seconds':9,'intake_end_seconds':1.2,'output_start_seconds':7.7,'collision_center_cm':[26,0,110],'collision_half_extent_cm':[100,110,110],'suggested_world_cm':[650,-530,2],'suggested_yaw':180}},
 'render_scope':'Original Blender studio views only; final in-engine material, light, motion and interaction review remains separate.'}
(OUT/'inventory.json').write_text(json.dumps(manifest,indent=2)+'\n')
if not opt.no_render:
    for label,frame in [('Open',75),('Closed',1)]:
        scene.frame_set(frame);camera_at((620,-650,520),(0,0,104),700)
        scene.render.filepath=str(OUT/('Blender_Machines_'+label+'.png'));bpy.ops.render.render(write_still=True)
    scene.frame_set(75);camera_at((440,-520,400),(0,-180,72),320)
    scene.render.filepath=str(OUT/'Blender_ContainmentChest_Detail.png');bpy.ops.render.render(write_still=True)
    camera_at((490,-240,410),(0,170,108),370)
    scene.render.filepath=str(OUT/'Blender_WeaponProcessor_Detail.png');bpy.ops.render.render(write_still=True)
print('C04 ORIGINAL MACHINE ASSETS COMPLETE',len(asset_rows),'meshes',sum(r['triangles'] for r in asset_rows.values()),'triangles')
