"""Project ONE original environment / carbine authoring. Blender 5.1, no inputs.

Run: blender --background --python Scripts/create_environment.py
Meters in authoring helpers are stored as centimetres in a .01 metric scene.
Preserves a complete editable .blend, one FBX per authored static mesh and a
deterministic original RGBA blood mask. No downloaded or previous-project art.
"""
from pathlib import Path
import bpy, math, random
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'ArtSource' / 'Environment'
OUT = ROOT / 'ArtSource' / 'Exports'
TEX = ROOT / 'ArtSource' / 'Textures'
for p in (SRC, OUT, TEX): p.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.render.resolution_x = 1600
scene.render.resolution_y = 1100
scene.render.resolution_percentage = 100

PALETTE = {
 'M_Concrete': ((.24,.27,.28,1), .94, .0),
 'M_Floor': ((.29,.32,.325,1), .9, .0),
 'M_FloorEdge': ((.08,.115,.125,1), .93, .05),
 'M_Graphite': ((.043,.065,.076,1), .78, .35),
 'M_Teal': ((.035,.16,.175,1), .72, .35),
 'M_PaleMetal': ((.52,.56,.54,1), .68, .48),
 'M_Steel': ((.19,.225,.24,1), .53, .7),
 'M_Rubber': ((.012,.018,.023,1), .92, .0),
 'M_Copper': ((.36,.18,.065,1), .66, .55),
 'M_Amber': ((.75,.32,.035,1), .69, .25),
 'M_LightAmber': ((1,.36,.05,1), .4, .0),
 'M_LightCool': ((.36,.82,.86,1), .48, .0),
 'M_Screen': ((.015,.055,.06,1), .61, .15),
 'M_Marking': ((.67,.70,.63,1), .83, .0),
 'M_BloodFlesh': ((.19,.009,.016,1), .7, .0),
}
MATS = {}
for name, (color, rough, metal) in PALETTE.items():
    m = bpy.data.materials.new(name); m.diffuse_color=color; m.use_nodes=True
    bsdf=m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value=color
    bsdf.inputs['Roughness'].default_value=rough
    bsdf.inputs['Metallic'].default_value=metal
    if name.startswith('M_Light'):
        bsdf.inputs['Emission Color'].default_value=color
        bsdf.inputs['Emission Strength'].default_value=2
    MATS[name]=m

parts=[]
assets=[]
def finish_obj(o, name, mat, bevel=0):
    o.name=name
    if mat: o.data.materials.append(MATS[mat])
    bpy.context.view_layer.objects.active=o
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Machined edge radii','BEVEL'); mod.width=bevel*100; mod.segments=2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if o.type=='MESH':
        for poly in o.data.polygons: poly.use_smooth=False
    parts.append(o)
    return o
def box(name, loc, size, mat, bevel=.01, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=tuple(v*100 for v in loc))
    o=bpy.context.object; o.dimensions=tuple(v*100 for v in size)
    o.rotation_euler=rot
    return finish_obj(o,name,mat,bevel)
def cyl(name,loc,r,depth,mat,axis='Z',vertices=24,bevel=.007):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=r*100, depth=depth*100, location=tuple(v*100 for v in loc))
    o=bpy.context.object
    if axis=='X': o.rotation_euler.y=math.pi/2
    if axis=='Y': o.rotation_euler.x=math.pi/2
    return finish_obj(o,name,mat,bevel)
def beam(name,a,b,r,mat,vertices=16):
    a,b=Vector(a)*100,Vector(b)*100; mid=(a+b)/2; d=b-a
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r*100,depth=d.length,location=mid)
    o=bpy.context.object; o.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    return finish_obj(o,name,mat,.004)
def textmesh(name,body,loc,size,mat,rot=(math.pi/2,0,0),align='CENTER'):
    cu=bpy.data.curves.new(name,'FONT'); cu.body=body; cu.align_x=align; cu.size=size*100
    cu.extrude=.07; cu.bevel_depth=.025
    o=bpy.data.objects.new(name,cu); bpy.context.collection.objects.link(o)
    o.location=Vector(loc)*100; o.rotation_euler=rot
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
    bpy.ops.object.convert(target='MESH')
    return finish_obj(bpy.context.object,name,mat)
def screws(y,xs,zs):
    for x in xs:
        for z in zs: cyl('Captive bolt',(x,y,z),.017,.016,'M_Steel','Y',8,.003)
def commit(name):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts: p.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join(); o=bpy.context.object; o.name=name
    scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    assets.append(o); parts=[]
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,
        object_types={'MESH'},use_mesh_modifiers=True,add_leaf_bones=False,
        axis_forward='-Y',axis_up='Z',apply_unit_scale=True,global_scale=1,
        mesh_smooth_type='FACE',bake_anim=False,use_custom_props=False)
    o.hide_set(True)
    return o

# One floor module: a full load-bearing slab, shallow joint and paired edge strips.
box('Cast substrate',(0,0,-.10),(4,4,.20),'M_Concrete',.025)
box('Sealed wearing surface',(0,0,-.015),(3.955,3.955,.055),'M_Floor',.017)
for x in (-1.98,1.98): box('Expansion joint',(x,0,.008),(.018,3.96,.006),'M_FloorEdge',0)
for y in (-1.98,1.98): box('Expansion joint',(0,y,.008),(3.96,.018,.006),'M_FloorEdge',0)
commit('SM_FloorModule')

# Rear/side architectural bay, 4m wide; visible front looks toward -Y.
box('Concrete structure',(0,0,1.5),(4,.30,3),'M_Concrete',.03)
box('Lower service chase',(0,-.20,.32),(3.85,.16,.62),'M_Teal',.018)
box('Impact rail',(0,-.305,.62),(3.92,.10,.11),'M_Steel',.012)
box('Upper cap',(0,-.19,2.94),(4,.22,.12),'M_Graphite',.015)
for x in (-1.90,0,1.90):
    box('Structural rib',(x,-.23,1.70),(.13,.19,2.43),'M_PaleMetal',.018)
for x in (-.96,.96):
    box('Insulated inner panel',(x,-.17,1.77),(1.68,.075,1.88),'M_PaleMetal',.02)
    box('Recessed service plate',(x,-.225,1.45),(1.18,.035,.75),'M_Concrete',.012)
    screws(-.251,(x-.52,x+.52),(1.14,1.76))
    for z in (2.40,2.48,2.56): box('Air slit',(x,-.228,z),(.96,.02,.029),'M_Graphite',.006)
box('Warning stripe',(0,-.30,.45),(3.75,.025,.065),'M_Amber',.004)
commit('SM_WallBay')

# Low cutaway wall retains a deliberate rail silhouette along the camera side.
box('Barrier base',(0,0,.28),(4,.28,.56),'M_Concrete',.025)
box('Protective teal panel',(0,-.16,.28),(3.87,.10,.45),'M_Teal',.015)
box('Rubber impact edge',(0,0,.60),(4,.38,.10),'M_Graphite',.025)
for x in (-1.91,1.91): box('Guardrail mount',(x,0,.46),(.14,.38,.79),'M_PaleMetal',.02)
commit('SM_CutawayBarrier')

# Containment pressure door, closed decorative architectural element.
box('Door casing',(0,0,1.45),(2.75,.36,2.9),'M_Graphite',.045)
for x in (-1.26,1.26): box('Locking jamb',(x,-.24,1.40),(.24,.24,2.76),'M_PaleMetal',.025)
box('Lintel',(0,-.24,2.79),(2.56,.26,.24),'M_PaleMetal',.025)
for x in (-.57,.57):
    box('Pressure door leaf',(x,-.23,1.37),(1.11,.16,2.55),'M_Teal',.027)
    box('Inset armored leaf',(x,-.335,1.50),(.87,.085,1.87),'M_Steel',.024)
    for z in (.57,2.25): box('Leaf reinforced rib',(x,-.40,z),(.91,.08,.10),'M_PaleMetal',.012)
    box('Door observation recess',(x,-.388,1.93),(.47,.025,.27),'M_Graphite',.009)
    box('Opaque safety glass',(x,-.41,1.93),(.39,.012,.18),'M_Screen',.005)
    box('Lock actuator',(x*.18,-.403,1.06),(.08,.09,.31),'M_Amber',.01)
box('Center pressure seal',(0,-.34,1.36),(.038,.12,2.60),'M_Rubber',.006)
box('Safety lintel lamp',(0,-.39,2.81),(.58,.028,.075),'M_LightAmber',.01)
textmesh('Door stencil','07  /  ISOLATION',(0,-.40,2.47),.14,'M_Marking')
commit('SM_PressureDoor')

# Personnel-scale lab console with authored panel layout and conduits.
box('Console plinth',(0,0,.10),(1.6,.74,.20),'M_Graphite',.035)
box('Console lower enclosure',(0,.04,.58),(1.44,.64,.98),'M_Teal',.05)
box('Worktop',(0,-.06,1.09),(1.73,.88,.105),'M_PaleMetal',.04)
box('Rear riser',(0,.22,1.22),(1.59,.18,.36),'M_Graphite',.027)
for x in (-.39,.39):
    box('Display bezel',(x,.108,1.31),(.68,.05,.35),'M_Graphite',.018)
    box('Active display',(x,.075,1.32),(.58,.023,.255),'M_Screen',.009)
    for n in range(4): box('Display telemetry',(x-.15,.058,1.40-n*.04),(.21+(n%2)*.14,.006,.012),'M_LightCool',.002)
    for n in range(3): box('Status indicators',(x+.19,.055,1.39-n*.06),(.028,.01,.028),'M_LightAmber' if n==2 else 'M_LightCool',.003)
for x in (-.46,0,.46):
    box('Equipment drawer',(x,-.299,.65),(.42,.035,.49),'M_PaleMetal',.012)
    box('Drawer pull',(x,-.344,.81),(.16,.055,.026),'M_Graphite',.009)
for n in range(10): box('Console keyboard',(-.40+n*.061,-.245,1.155),(.042,.16,.026),'M_Graphite',.004)
cyl('Emergency stop',(.59,-.18,1.17),.065,.05,'M_Amber')
beam('External conduit',(.63,.31,.12),(.63,.31,1.12),.035,'M_Rubber')
commit('SM_LabConsole')

# Tall rear-wall power rack; repeated fins/readable equipment, not a box proxy.
box('Rack foot',(0,0,.075),(1.0,.70,.15),'M_Graphite',.025)
box('Service cabinet',(0,0,1.03),(.92,.62,1.97),'M_PaleMetal',.035)
box('Inset teal front',(0,-.333,1.15),(.76,.065,1.66),'M_Teal',.018)
for x in (-.40,.40): box('Rack mounting column',(x,-.38,1.09),(.08,.10,1.87),'M_Graphite',.009)
for i in range(6):
    z=.43+i*.235
    box('Removable supply unit',(0,-.40,z),(.67,.14,.19),'M_Graphite',.008)
    for j in range(6): box('Cooling aperture',(-.22+j*.062,-.482,z),(.031,.016,.092),'M_Steel',.003)
    cyl('Rack unit indicator',(.25,-.484,z),.017,.012,'M_LightAmber' if i==4 else 'M_LightCool','Y',12,.001)
box('Power switch panel',(0,-.403,1.83),(.64,.04,.16),'M_Screen',.008)
textmesh('Power stencil','AUX  /  480V',(0,-.432,1.80),.078,'M_Marking')
beam('Power feed',(.38,.18,1.76),(.38,.18,2.27),.043,'M_Rubber')
beam('Power bend',(.38,.18,2.27),(.12,.18,2.36),.043,'M_Rubber')
commit('SM_PowerRack')

# Pressure vessel with saddles, head rings, valve, gauge, connection pipes.
box('Vessel skid',(0,0,.08),(1.27,1.13,.16),'M_Graphite',.028)
for x in (-.43,.43): box('Tank saddle',(x,0,.31),(.17,.89,.54),'M_Steel',.035)
cyl('Cylindrical vessel',(0,0,1.15),.49,1.50,'M_PaleMetal',vertices=32)
for z in (.40,.48,1.80,1.89): cyl('Clamp band',(0,0,z),.525,.052,'M_Teal',vertices=32)
cyl('Top crown',(0,0,1.91),.46,.16,'M_Steel',vertices=32)
cyl('Vessel top actuator',(0,0,2.08),.12,.20,'M_Copper')
for a in range(6):
    ang=a*math.tau/6; beam('Valve spoke',(0,0,2.20),(.21*math.cos(ang),.21*math.sin(ang),2.20),.018,'M_Amber')
bpy.ops.mesh.primitive_torus_add(major_radius=21,minor_radius=1.8,major_segments=24,minor_segments=8,location=(0,0,220))
finish_obj(bpy.context.object,'Handwheel','M_Amber')
cyl('Gauge housing',(0,-.51,1.47),.116,.085,'M_Graphite','Y')
cyl('Gauge face',(0,-.56,1.47),.092,.018,'M_Marking','Y')
beam('Gauge needle',(0,-.576,1.47),(.046,-.576,1.53),.009,'M_Graphite',8)
beam('Feed pipe',(.42,0,.58),(.71,0,.58),.07,'M_Steel')
beam('Feed riser',(.71,0,.58),(.71,0,1.65),.07,'M_Steel')
box('Identification band',(0,-.499,1.07),(.40,.05,.22),'M_Teal',.005)
textmesh('Tank ID','N2 / 07',(0,-.535,1.015),.075,'M_Marking')
commit('SM_PressureVessel')

# Low research bench creates pathfinding variation without hiding actors.
for x in (-1.00,1.00):
    box('Splayed bench pedestal',(x,0,.37),(.34,.81,.74),'M_Teal',.032)
    box('Pedestal foot',(x,0,.10),(.50,.97,.16),'M_Graphite',.025)
box('Containment bench deck',(0,0,.80),(2.72,1.03,.18),'M_PaleMetal',.035)
box('Recessed instrument surface',(0,0,.90),(2.30,.73,.035),'M_Graphite',.015)
for y in (-.44,.44): box('Bench retaining lip',(0,y,.95),(2.58,.04,.10),'M_Steel',.009)
for x in (-1.18,1.18):
    box('Bench end rail',(x,0,.97),(.07,.86,.13),'M_Amber',.012)
for x in (-.90,-.52,.52,.90):
    box('Instrument cradle',(x,0,.96),(.15,.42,.08),'M_Steel',.014)
box('Sealed field case',(0,0,1.05),(.67,.44,.28),'M_Teal',.028)
for x in (-.20,.20): box('Case latch',(x,-.234,1.07),(.055,.035,.09),'M_PaleMetal',.006)
commit('SM_ResearchBench')

# Sealed maintenance floor hatch; a thin authored decorative mesh.
box('Recess frame',(0,0,.01),(1.22,1.22,.025),'M_Graphite',.012)
box('Hatch plate',(0,0,.025),(1.13,1.13,.033),'M_Steel',.014)
for x in (-.40,.40):
    box('Recessed lifting handle',(x,0,.046),(.12,.37,.018),'M_Graphite',.01)
    box('Handle center',(x,0,.058),(.07,.20,.028),'M_PaleMetal',.007)
for x in (-.50,.50):
    for y in (-.50,.50): cyl('Hatch fastener',(x,y,.05),.027,.015,'M_Graphite',vertices=8)
commit('SM_ServiceHatch')

# Long wall luminaire and illuminated endcap; controllable source lights in UE.
box('Light mounting channel',(0,0,0),(1.7,.13,.20),'M_Graphite',.017)
box('Frosted strip',(0,-.079,0),(1.50,.025,.105),'M_LightCool',.009)
for x in (-.80,.80): box('Luminaire end cap',(x,-.025,0),(.13,.19,.24),'M_PaleMetal',.012)
commit('SM_WallLight')

box('Information sign',(0,0,0),(3.48,.08,.65),'M_Graphite',.018)
box('Sign index',(-1.47,-.052,0),(.37,.03,.46),'M_Teal',.01)
textmesh('Index glyph','07',(-1.47,-.077,-.08),.21,'M_Marking')
textmesh('Facility ID','CONTAINMENT',(.13,-.055,.035),.23,'M_Marking')
textmesh('Facility subtitle','RESEARCH  /  LOWER SECTOR B',(.13,-.055,-.18),.088,'M_Marking')
commit('SM_ContainmentSign')

# Painted floor wayfinding preserves open combat space and gives the central
# gameplay view a facility identity without more obstructing props.
for x in (-3.15,3.15):
    for y in (-1.7,1.7): box('Safety zone corner',(x,y,.014),(.045,1.15,.007),'M_Marking',0)
for y in (-2.25,2.25):
    for x in (-2.65,2.65): box('Safety zone corner',(x,y,.014),(1.04,.045,.007),'M_Marking',0)
for x in (-3.12,3.12):
    for i in range(5): box('Equipment caution mark',(x,-.52+i*.24,.015),(.12,.12,.008),'M_Amber',0,rot=(0,0,-.4))
textmesh('Floor zone stencil','B-07  /  CONTAINMENT',(0,-3.00,.02),.29,'M_Marking',rot=(0,0,0))
textmesh('Floor zone instruction','ISOLATION FLOOR    /    KEEP CLEAR',(0,-3.24,.02),.10,'M_Marking',rot=(0,0,0))
for x in (-.50,.50):
    beam('Directional arrow',(x,2.9,.022),(x+.24,2.64,.022),.025,'M_Amber',8)
    beam('Directional arrow',(x,2.9,.022),(x-.24,2.64,.022),.025,'M_Amber',8)
commit('SM_FloorWayfinding')

# Original response carbine. Grip origin is (0,0,0), +X forward, muzzle(.58,0,.14)m.
box('Upper receiver',(.042,0,.123),(.30,.075,.092),'M_Graphite',.014)
box('Lower receiver',(-.008,0,.07),(.195,.066,.070),'M_Steel',.008)
box('Handguard',(.267,0,.132),(.24,.065,.083),'M_Teal',.013)
box('Barrel saddle',(.385,0,.138),(.045,.080,.095),'M_Graphite',.008)
cyl('Barrel',(.462,0,.14),.015,.17,'M_Steel','X',16,.002)
cyl('Muzzle brake',(.561,0,.14),.024,.057,'M_Graphite','X',12,.003)
cyl('Crowned muzzle',(.591,0,.14),.014,.006,'M_Rubber','X',12,.001)
for x in (.545,.56,.575):
    for y in (-.022,.022): box('Muzzle side ports',(x,y,.14),(.006,.009,.019),'M_Rubber',.001)
box('Full length rail',(.105,0,.183),(.48,.039,.016),'M_Steel',.003)
for i in range(24): box('Picatinny cross rib',(-.12+i*.020,0,.195),(.011,.046,.012),'M_Graphite',.002)
for side in (-1,1):
    for i in range(6): box('Handguard cooling slot',(.18+i*.029,side*.035,.138),(.018,.008,.025),'M_Rubber',.004)
box('Pistol grip',(-.027,0,-.015),(.057,.047,.13),'M_Rubber',.009,rot=(0,-.22,0))
for i in range(5): box('Grip texture',(-.016,0,-.04+i*.017),(.057,.049,.004),'M_Graphite',.001,rot=(0,-.22,0))
beam('Trigger guard back',(-.01,0,.035),(.076,0,-.011),.009,'M_Graphite',8)
beam('Trigger guard floor',(.02,0,-.005),(.08,0,-.005),.009,'M_Graphite',8)
beam('Trigger guard front',(.08,0,-.005),(.093,0,.055),.009,'M_Graphite',8)
beam('Trigger',(.045,0,.051),(.034,0,.015),.007,'M_Steel',8)
box('Magazine',(.135,0,-.02),(.079,.047,.177),'M_Graphite',.01,rot=(0,-.16,0))
for z in (-.085,-.055,-.025): box('Magazine pressed rib',(.140,.025,z),(.044,.006,.008),'M_Steel',.002)
cyl('Stock buffer',(-.194,0,.122),.022,.17,'M_Steel','X',16,.003)
box('Adjustable stock',(-.282,0,.105),(.19,.070,.111),'M_Teal',.017)
box('Stock cheekpiece',(-.258,0,.166),(.14,.062,.034),'M_Graphite',.009)
box('Rubber buttpad',(-.383,0,.088),(.032,.077,.155),'M_Rubber',.01)
box('Ejection port',(.032,.040,.135),(.102,.009,.031),'M_Rubber',.004)
box('Ejection bolt',(.036,.046,.137),(.065,.005,.017),'M_Steel',.003)
cyl('Selector',(-.057,.04,.081),.017,.008,'M_Steel','Y',12,.002)
box('Optic mounting foot',(-.016,0,.211),(.095,.047,.025),'M_Graphite',.004)
box('Reflex hood',(-.013,0,.243),(.079,.063,.055),'M_Teal',.008)
box('Reflex lens',(.029,0,.245),(.006,.048,.038),'M_Screen',.002)
box('Front sight',(.323,0,.212),(.024,.028,.039),'M_Graphite',.004)
commit('SM_Carbine')

# A deterministic original decal alpha: broad pool with asymmetric satellite drops.
random.seed(1707)
N=256
im=bpy.data.images.new('T_BloodMask',width=N,height=N,alpha=True)
drops=[(0,0,.47,.30,.16)]
for i in range(23):
    a=random.uniform(0,math.tau); r=random.uniform(.30,.83)
    drops.append((math.cos(a)*r,math.sin(a)*r,random.uniform(.016,.092),random.uniform(.022,.083),random.uniform(-2,2)))
pix=[]
for j in range(N):
    y=(j/(N-1)-.5)*2
    for i in range(N):
        x=(i/(N-1)-.5)*2; alpha=0.
        for cx,cy,rx,ry,angle in drops:
            dx=x-cx;dy=y-cy;u=(dx*math.cos(angle)+dy*math.sin(angle))/rx;v=(-dx*math.sin(angle)+dy*math.cos(angle))/ry
            r=math.sqrt(u*u+v*v); a=math.atan2(v,u)
            edge=1+.06*math.sin(a*7)+.035*math.sin(a*13)
            alpha=max(alpha,max(0,min(1,(edge-r)*18)))
        pix.extend((1,1,1,alpha))
im.pixels=pix; im.filepath_raw=str(TEX/'T_BloodMask.png'); im.file_format='PNG'; im.save()

# Store assets spread apart in an editable catalog for convenient Blender review.
for i,o in enumerate(assets):
    o.hide_set(False); o.location=((i%4)*470,(i//4)*470,0)
scene.world.color=(.25,.25,.25)
bpy.ops.wm.save_as_mainfile(filepath=str(SRC/'ProjectONE_IndustrialKit.blend'))
manifest={'author':'Original procedural modeling by Codex for Project ONE','third_party_assets':False,'units':'centimetres','forward':'+X carbine; wall facade -Y','fbx':{'axis_forward':'-Y','axis_up':'Z','apply_unit_scale':True,'global_scale':1},'assets':[o.name for o in assets],'palette':PALETTE}
import json
(SRC/'manifest.json').write_text(json.dumps(manifest,indent=2))
print('PROJECT ONE ENVIRONMENT COMPLETE:',len(assets),'original static assets')
