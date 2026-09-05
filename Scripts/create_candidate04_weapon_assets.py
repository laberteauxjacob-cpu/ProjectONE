"""Original Candidate04 weapon models and actions on the accepted Response rig.

Blender 5.1.2 --background --python-exit-code 1 --python this_script
No downloaded geometry, textures, recordings, or changes to accepted C01-C03 assets.
All static parts are centimetres, +X barrel, +Z up, with the same grip-centred origin.
Run the portable metadata sanitizer after generation and before import/publication.
"""
from pathlib import Path
import bpy, bmesh, json, math, sys
from mathutils import Vector, Matrix, Quaternion

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ArtSource/Weapons/Candidate04'
EXPORT = ROOT / 'ArtSource/Exports/Candidate04'
for directory in (SOURCE, EXPORT): directory.mkdir(parents=True, exist_ok=True)
FPS = 100
PALETTE = {
    'M_C04_BluedSteel': ([.055,.069,.080,1], .43,.78, 0),
    'M_C04_Parkerized': ([.10,.115,.12,1], .69,.58, 0),
    'M_C04_EdgeSteel': ([.27,.30,.31,1], .42,.78, 0),
    'M_C04_Polymer': ([.025,.030,.033,1], .88,.0, 0),
    'M_C04_Walnut': ([.22,.095,.042,1], .75,.0, 0),
    'M_C04_Bore': ([.008,.012,.014,1], .94,.0, 0),
    'M_C04_Brass': ([.48,.32,.10,1], .38,.78, 0),
    'M_C04_Ivory': ([.66,.69,.59,1], .66,.1, 0),
    'M_C04_VioletArmor': ([.085,.034,.13,1], .43,.7, 0),
    'M_C04_CyanArmor': ([.025,.115,.13,1], .47,.68, 0),
    'M_C04_EmberArmor': ([.15,.063,.024,1], .52,.7, 0),
    'M_C04_VioletEnergy': ([.43,.09,1,1], .36,.25, 4.0),
    'M_C04_CyanEnergy': ([.025,.72,1,1], .36,.25, 3.5),
    'M_C04_EmberEnergy': ([1,.24,.025,1], .36,.25, 3.8),
}
MATS = {}
PARTS = []
STATIC = {}
ASSEMBLIES = {}
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'; scene.unit_settings.scale_length = .01
scene.render.fps = FPS
for name,(color,roughness,metallic,emission) in PALETTE.items():
    mat = bpy.data.materials.new(name); mat.use_nodes = True; mat.diffuse_color = color
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value=color
    bsdf.inputs['Roughness'].default_value=roughness
    bsdf.inputs['Metallic'].default_value=metallic
    bsdf.inputs['Emission Color'].default_value=color
    bsdf.inputs['Emission Strength'].default_value=emission
    MATS[name] = mat

def finish(obj, name, mat='M_C04_BluedSteel', bevel=.12):
    obj.name=name; obj.data.materials.append(MATS[mat])
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod=obj.modifiers.new('Machined edge radius','BEVEL');mod.width=bevel;mod.segments=3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    PARTS.append(obj)
    return obj

def box(name, pos, size, mat='M_C04_BluedSteel', bevel=.12, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
    obj=bpy.context.object;obj.dimensions=size;obj.rotation_euler=rot
    return finish(obj,name,mat,bevel)

def cyl(name,pos,radius,length,mat='M_C04_BluedSteel',axis='X',verts=28,bevel=.06):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=length,location=pos)
    obj=bpy.context.object
    if axis=='X':obj.rotation_euler.y=math.pi/2
    elif axis=='Y':obj.rotation_euler.x=math.pi/2
    return finish(obj,name,mat,bevel)

def beam(name,a,b,radius,mat='M_C04_BluedSteel',verts=12):
    a,b=Vector(a),Vector(b);d=b-a
    obj=cyl(name,(a+b)/2,radius,d.length,mat,'Z',verts,.035)
    obj.rotation_euler=d.to_track_quat('Z','Y').to_euler()
    return obj

def profile(name, outline, width, mat='M_C04_BluedSteel', bevel=.1, y=0):
    """Closed side-profile extrusion; outlines are original measured design points."""
    count=len(outline)
    vertices=[(x,y+sign*width/2,z) for sign in (-1,1) for x,z in outline]
    faces=[tuple(reversed(range(count))),tuple(range(count,2*count))]
    faces += [(i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj)
    bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
    return finish(obj,name,mat,bevel)

def hollow_tube(name,x0,x1,z,radius,wall,mat='M_C04_BluedSteel',y=0):
    n=32
    vertices=[(x,y+r*math.cos(i*math.tau/n),z+r*math.sin(i*math.tau/n))
              for x,r in [(x0,radius),(x1,radius),(x1,radius-wall),(x0,radius-wall)] for i in range(n)]
    faces=[]
    for j in range(4):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,((j+1)%4)*n+(i+1)%n,((j+1)%4)*n+i))
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
    obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj)
    return finish(obj,name,mat,0)

def boolean_opening(obj, pos, size, name):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos);cutter=bpy.context.object;cutter.dimensions=size
    bpy.context.view_layer.objects.active=cutter;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    bpy.context.view_layer.objects.active=obj
    mod=obj.modifiers.new(name,'BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)

def export_fbx(path, objects, animated=False):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects: obj.hide_set(False);obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','ARMATURE'},
        axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
        use_space_transform=True,bake_space_transform=False,add_leaf_bones=False,primary_bone_axis='Y',secondary_bone_axis='X',
        use_armature_deform_only=False,mesh_smooth_type='FACE',use_mesh_modifiers=True,bake_anim=animated,
        bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True,bake_anim_simplify_factor=0)

def commit(name, role):
    global PARTS
    bpy.ops.object.select_all(action='DESELECT')
    for obj in PARTS:obj.select_set(True)
    bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();obj=bpy.context.object;obj.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    mesh=obj.data
    for index,material in enumerate(mesh.materials):
        if material is None:mesh.materials[index]=MATS['M_C04_BluedSteel']
    bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
    # Original subtle surface variation, carried as vertex colour rather than images.
    color=mesh.color_attributes.get('Color') or mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    for poly in mesh.polygons:
        for loop in poly.loop_indices:
            v=mesh.vertices[mesh.loops[loop].vertex_index].co
            f=.92+.08*abs(math.sin(v.x*1.7+v.z*.77+v.y*.33))
            color.data[loop].color=(f,f,f,1)
    # Explicit UVs prevent tangent/import degeneracy; source materials use vertex colour.
    uv=mesh.uv_layers.new(name='UVMap') if not mesh.uv_layers else mesh.uv_layers.active
    for poly in mesh.polygons:
        ax=max(range(3),key=lambda i:abs(poly.normal[i]));axes=[i for i in range(3) if i!=ax]
        for loop in poly.loop_indices:
            v=mesh.vertices[mesh.loops[loop].vertex_index].co;uv.data[loop].uv=(v[axes[0]]*.05,v[axes[1]]*.05)
    mesh.calc_loop_triangles()
    lo=[min(v.co[i] for v in mesh.vertices) for i in range(3)]
    hi=[max(v.co[i] for v in mesh.vertices) for i in range(3)]
    assert all(math.isfinite(n) for v in mesh.vertices for n in v.co)
    STATIC[name]={'source':f'ArtSource/Exports/Candidate04/{name}.fbx','asset':f'/Game/ONE/Art/Weapons/Candidate04/{name}',
        'role':role,'bounds_source_cm':[lo,hi],'vertices':len(mesh.vertices),'triangles':len(mesh.loop_triangles),
        'material_slots':[m.name for m in mesh.materials],'texture_dependencies':[]}
    PARTS=[];export_fbx(EXPORT/(name+'.fbx'),[obj]);return obj

def pistol(upgraded=False):
    family='LastWord' if upgraded else 'M1911';armor='M_C04_VioletArmor' if upgraded else 'M_C04_BluedSteel'
    grip='M_C04_Polymer' if upgraded else 'M_C04_Walnut'
    profile('1911 forged frame',[(-4,3.7),(15.5,3.7),(15.5,2.6),(8.1,2.1),(5.3,1.3),(2.7,-7.1),(-2.6,-7.5),(-.2,1.2),(-4,2.1)],2.7,armor,.16)
    profile('Magazine well',[(.1,2.0),(4.4,1.0),(1.8,-7.6),(-2.4,-7.8)],2.85,'M_C04_Parkerized',.10)
    # Open trigger guard is a closed loop, not a solid box.
    for a,b in [((4.9,0,2.8),(8.2,0,2.0)),((8.2,0,2.0),(8.4,0,-.8)),((8.4,0,-.8),(3.8,0,-1.3))]:beam('Curved trigger guard',a,b,.33,armor,16)
    profile('Short curved trigger',[(5.3,2.4),(6.1,2.4),(6.3,.8),(5.8,.0),(5.4,.3)],.85,'M_C04_EdgeSteel',.08)
    for side in (-1,1):
        profile('Checkered grip panel',[(-.1,.6),(3.7,.1),(1.3,-6.9),(-2.1,-7.1)],.36,grip,.2,y=side*1.54)
        for i in range(12):
            z=-6.5+i*.53;x=-.6+(z+6.5)*.30
            box('Fine grip checkering',(x,side*1.76,z),(2.0,.09,.14),'M_C04_BluedSteel',.035,rot=(0,.30,0))
        for x,z in [(1.6,-.1),(-.5,-6.0)]:
            cyl('Grip screw',(x,side*1.80,z),.27,.13,'M_C04_EdgeSteel','Y',16,.025)
            box('Screw slot',(x,side*1.88,z),(.35,.025,.055),'M_C04_Bore',.012)
    profile('Grip safety beavertail',[(-4.9,3.5),(-2.3,3.8),(-.6,2.0),(-1.1,1.1),(-2.4,2.2),(-4.7,2.5)],1.35,'M_C04_BluedSteel',.12)
    profile('Spur hammer',[(-5.7,4.5),(-4.5,5.9),(-3.7,5.8),(-3.3,4.4),(-4.0,3.0)],.65,'M_C04_EdgeSteel',.10)
    box('Thumb safety',(-1.8,-1.57,2.2),(2.1,.32,.55),'M_C04_EdgeSteel',.09)
    box('Slide stop',(2.1,-1.6,2.7),(3.1,.35,.5),'M_C04_EdgeSteel',.10)
    cyl('Magazine release',(3.5,-1.66,.35),.36,.28,'M_C04_EdgeSteel','Y',20,.025)
    hollow_tube('Barrel and open bore',4.8,16.5,5.5,.70,.19,'M_C04_EdgeSteel')
    cyl('Recoil spring plug',(15.4,0,3.6),.48,1.0,'M_C04_EdgeSteel',verts=24)
    if upgraded:
        for side in (-1,1):
            profile('Violet receiver inset',[(7.1,2.8),(13.7,2.8),(14.5,3.5),(7.5,3.5)],.10,'M_C04_VioletEnergy',.035,y=side*1.39)
            for x in (0,.65,1.3):box('Grip charge index',(x,side*1.81,-2.8),( .3,.12,1.15),'M_C04_VioletEnergy',.035)
    body=commit('SM_'+family+'_Body','1911 frame, fixed barrel, controls and grip; slide and seated magazine separate')
    # Rounded slide roof, flat rails and a real open right-side ejection cut.
    slide=profile('1911 slide',[(-4.7,3.8),(16.15,3.8),(16.15,6.5),(15.5,7.1),(-4.2,7.1),(-4.7,6.5)],2.72,armor,.18)
    boolean_opening(slide,(5.7,1.4,6.3),(4.1,2.3,2.8),'Open ejection port')
    boolean_opening(slide,(15.95,0,5.5),(1.0,1.5,1.5),'Barrel bushing opening')
    for side in (-1,1):
        for i in range(9):box('Rear slide serration',(-3.8+i*.41,side*1.40,5.65),(.11,.08,1.9),'M_C04_Bore',.02)
        box('Slide machined bevel',(9.8,side*1.37,6.75),(8.0,.08,.12),'M_C04_EdgeSteel',.03)
    box('Rear sight base',(-3.25,0,7.2),(1.4,2.2,.35),'M_C04_Parkerized',.08)
    for y in (-.84,.84):box('Rear notch blade',(-3.3,y,7.68),(.5,.50,.75),'M_C04_Parkerized',.05)
    box('Front sight',(14.8,0,7.50),(1.2,.3,.55),'M_C04_Parkerized',.06)
    if upgraded:
        for side in (-1,1):
            box('Selective slide energy trace',(9.4,side*1.43,5.2),(7.0,.12,.25),'M_C04_VioletEnergy',.045)
            for i in range(3):box('Violet slide vent',(-2.8+i*.7,side*1.44,6.5),(.32,.12,.40),'M_C04_VioletEnergy',.035)
        box('Violet front sight',(14.6,0,7.78),(.45,.32,.10),'M_C04_VioletEnergy',.025)
    moving=commit('SM_'+family+'_Slide','1911 reciprocating slide; identity seated, translate -3X at full recoil')
    profile('Single stack magazine',[(.0,1.8),(3.6,.6),(1.15,-7.6),(-2.2,-7.8)],1.70,'M_C04_EdgeSteel',.08)
    profile('Magazine floorplate',[(-2.55,-7.7),(1.55,-7.5),(1.7,-8.05),(-2.7,-8.25)],2.0,armor,.10)
    for side in (-1,1):
        for i in range(5):cyl('Magazine witness hole',(.55-i*.22,side*.866,-1.0-i*1.05),.14,.06,'M_C04_Bore','Y',12,.012)
    if upgraded:box('Magazine violet floor index',(-.6,-1.03,-7.88),(1.8,.10,.20),'M_C04_VioletEnergy',.03)
    mag=commit('SM_'+family+'_Magazine','Matching cosmetic dropped/fresh/seated pistol magazine; common gun origin')
    ASSEMBLIES[family]=[body.name,moving.name,mag.name]

def rifle(upgraded=False):
    family='Overcurrent' if upgraded else 'M4A1';armor='M_C04_CyanArmor' if upgraded else 'M_C04_Parkerized'
    # Existing Project ONE grip/support/magazine interfaces, refined forged receiver.
    profile('M4 upper receiver',[(-11,10.0),(16.8,10.0),(17.5,13.0),(16.0,16.9),(-8.6,16.9),(-11,15.0)],5.7,armor,.42)
    profile('M4 lower receiver',[(-8.7,10.4),(16.0,10.4),(17.0,4.9),(8.7,3.1),(7.2,5.8),(-5.4,5.8),(-8.7,7.0)],5.2,armor,.32)
    profile('M4 magwell',[(8.5,9.6),(17,9.6),(17,2.6),(9.1,2.5)],5.8,armor,.36)
    profile('Slanted pistol grip',[(-4,6.0),(1.0,4.4),(-1.2,-8.5),(-6.2,-7.6)],4.9,'M_C04_Polymer',.5)
    for i in range(7):box('Grip molded stipple',(-2.5,0,-6.4+i*1.2),(4.9,4.97,.16),'M_C04_Parkerized',.06,rot=(0,-.16,0))
    for a,b in [((-.5,0,4),(1.6,0,-.4)),((1.6,0,-.4),(7.8,0,-.4)),((7.8,0,-.4),(9,0,4.5))]:beam('M4 trigger guard',a,b,.55,armor)
    beam('M4 trigger',(4.4,0,5.2),(3.6,0,1.1),.36,'M_C04_EdgeSteel')
    cyl('Buffer tube',(-20.2,0,13),1.9,20.5,'M_C04_EdgeSteel')
    profile('M4 sliding stock',[(-33.3,17),(-18.3,17),(-18.0,13.4),(-23,11.6),(-27,4),(-33.3,2)],6.6,'M_C04_Polymer',.55)
    profile('Open stock lower brace',[(-32,3),(-23,4.5),(-17.4,11.8),(-20.4,11.8),(-26,6.6),(-32,6)],3.0,armor,.30)
    box('Stock recoil pad',(-33.9,0,9.4),(1.2,7.1,15.6),'M_C04_Polymer',.35)
    for z in [3.2,5.5,7.8,10.1,12.4,14.7]:box('Stock traction',(-34.53,0,z),(.10,6.4,.28),'M_C04_Parkerized',.035)
    box('Stock adjustment latch',(-22.7,0,6.5),(6.0,3.7,1.0),'M_C04_Parkerized',.22)
    cyl('Delta ring',(15.7,0,14),3.8,1.7,armor)
    # Carbine-length quad handguard with ribs/vent windows, recognizably shorter than barrel.
    profile('M4 handguard',[(16.7,10.6),(33.1,10.6),(33.1,17.2),(16.7,17.2)],6.4,'M_C04_Polymer',.55)
    for side in (-1,1):
        for i in range(7):box('Handguard vent',(18.1+i*2.05,side*3.24,14.2),(1.18,.15,2.1),'M_C04_Bore',.20)
        box('Handguard side rail',(25,side*3.64,12.3),(15.0,.6,1.0),armor,.15)
        for i in range(8):box('Rail cross tooth',(17.9+i*2,side*3.94,12.3),(.78,.24,1.35),armor,.06)
    cyl('Barrel',(42.6,0,14),1.03,19.2,'M_C04_Parkerized')
    cyl('Stepped barrel shoulder',(34.5,0,14),1.40,2.2,'M_C04_BluedSteel')
    hollow_tube('A2 flash hider',51.4,54.5,14,1.47,.62,'M_C04_BluedSteel')
    for y in (-1.42,1.42):
        for x in [52.3,53.25]:box('A2 hider slot',(x,y,14),(.45,.13,1.25),'M_C04_Bore',.055)
    # Characteristic A-frame front sight and bayonet lug distinguish the old generic rifle.
    for side in (-1,1):
        beam('Front sight A rear',(33.7,side*1.2,14.3),(35.0,side*.55,22.2),.42,armor)
        beam('Front sight A front',(38,side*1.2,14.3),(35.0,side*.55,22.2),.42,armor)
    box('Front sight bridge',(35,0,21.8),(1.3,2.6,.65),armor,.15)
    cyl('Front sight post',(35,0,23),.21,1.55,'M_C04_EdgeSteel','Z',16,.02)
    box('Bayonet lug',(35.1,0,11.5),(3.5,2.0,1.1),armor,.20)
    box('Receiver rail',(2.2,0,17.8),(25.4,3.9,1.0),armor,.15)
    for i in range(13):box('Receiver rail tooth',(-9.8+i*2,0,18.4),(1.0,4.3,.55),armor,.06)
    # Compact removable carry handle with an actual gap.
    for x in (-7.3,8.0):box('Carry handle riser',(x,0,19.9),(2.0,3.5,2.8),armor,.20)
    box('Carry handle beam',(.4,0,21.35),(17.5,2.6,1.2),armor,.25)
    box('Rear sight housing',(-6.8,0,22.35),(3.0,3.4,1.7),armor,.20)
    box('Rear sight aperture',(-7.2,0,23.3),(.4,1.2,.9),'M_C04_Bore',.05)
    box('Ejection recess',(3.0,2.91,14),(8.0,.16,2.5),'M_C04_Bore',.18)
    box('Dust cover',(3.0,3.12,12.55),(8.0,.23,1.3),armor,.16)
    cyl('Forward assist',(-6,3.5,14.0),.9,3.2,'M_C04_Parkerized','Y',20,.13)
    profile('Case deflector',[(-2,12.6),(.2,13.8),(-1.7,16.1),(-3.1,15.3)],1.0,armor,.15,y=3.1)
    cyl('Selector',(-4,-2.8,9),.7,.5,'M_C04_EdgeSteel','Y',20,.05)
    box('Selector lever',(-4.8,-3.08,9),(2.0,.3,.4),'M_C04_EdgeSteel',.09)
    box('Bolt catch',(3,-2.9,10.3),(1.7,.5,2.4),'M_C04_EdgeSteel',.18)
    box('Charging handle',(-10.5,0,16.5),(1.5,6.1,.6),'M_C04_EdgeSteel',.12)
    if upgraded:
        for side in (-1,1):
            box('Receiver cyan channel',(4.0,side*2.98,15.9),(13.0,.13,.30),'M_C04_CyanEnergy',.05)
            for x in [18.8,23.8,28.8]:box('Cyan insulated handguard window',(x,side*3.38,15.8),(2.4,.18,.55),'M_C04_CyanEnergy',.09)
            box('Stock charge gauge',(-27,side*3.37,13.5),(4.8,.12,.40),'M_C04_CyanEnergy',.055)
    body=commit('SM_'+family+'_Body','M4A1-pattern receiver, carbine quad rail, A-frame sight and sliding stock; separate curved magazine')
    profile('Curved STANAG magazine',[(9.6,7.0),(16.9,6.8),(16.1,-3.0),(19.0,-12.8),(13.1,-15.0),(9.8,-6.0)],4.6,armor,.35)
    profile('Magazine baseplate',[(12.9,-14.4),(19.1,-12.4),(19.5,-13.2),(13.0,-15.4)],5.0,'M_C04_Polymer',.20)
    for side in (-1,1):
        for i in range(3):
            x=11.3+i*1.8
            beam('Magazine stamped flute',(x,side*2.37,4.4),(x+.2,side*2.37,-4.0),.16,'M_C04_EdgeSteel',8)
            beam('Magazine lower flute',(x+.2,side*2.37,-4),(x+2.3,side*2.37,-12),.16,'M_C04_EdgeSteel',8)
        if upgraded:box('Magazine cyan floor marking',(15.8,side*2.48,-12.5),(2.5,.12,.4),'M_C04_CyanEnergy',.04)
    mag=commit('SM_'+family+'_Magazine','Matching curved rifle magazine; cosmetic dropped/fresh/seated part with common gun origin')
    ASSEMBLIES[family]=[body.name,mag.name]

def shotgun(upgraded=False):
    family='Gravebreaker' if upgraded else 'Remington870';armor='M_C04_EmberArmor' if upgraded else 'M_C04_BluedSteel'
    # Preserve accepted receiver/fore-end/grip interfaces while giving stock its conventional 870 line.
    profile('870 rounded receiver',[(-9,7.0),(14.5,7),(16,9),(15.7,15.5),(13.7,17),(-7.5,17),(-10,14)],6.7,armor,.7)
    profile('870 trigger plate web',[(-7,9),(13,9),(13,4.7),(-5,4.7)],5.6,armor,.25)
    cyl('870 barrel',(39,0,14),1.22,50,'M_C04_Parkerized',verts=36)
    hollow_tube('870 open muzzle',63.4,64.5,14,1.35,.40,'M_C04_BluedSteel')
    cyl('870 tube magazine',(32,0,7),1.55,43,'M_C04_Parkerized')
    cyl('Tube end cap',(53.7,0,7),1.85,1.7,armor)
    box('Barrel tube clamp',(51.0,0,10.5),(1.35,3.65,7.7),armor,.35)
    for y in (-1.91,1.91):cyl('Clamp screw',(51,y,10.5),.37,.20,'M_C04_EdgeSteel','Y',16,.03)
    box('Ejection port',(5,3.43,13.5),(8.9,.18,3.2),'M_C04_Bore',.32)
    box('Visible bolt',(5,3.58,13.4),(6.8,.12,2.1),'M_C04_EdgeSteel',.15)
    box('Loading port',(8,0,4.42),(8.4,4.7,.18),'M_C04_Bore',.28)
    box('Loading gate',(7.8,0,4.24),(6.8,3.7,.16),'M_C04_Parkerized',.15)
    profile('Conventional 870 stock',[(-40,16),(-22,17),(-10,14),(-6.5,10),(-.6,4),(-3,-1.8),(-8,0),(-15,6),(-25,7),(-40,1.4)],6.8,'M_C04_Walnut' if not upgraded else 'M_C04_Polymer',.85)
    profile('Cheek comb',[(-38,16.1),(-22,17.2),(-13,14.9),(-16,13.4),(-37,13.8)],6.4,armor,.45)
    box('870 recoil pad',(-41,0,8.7),(1.8,7.5,16.3),'M_C04_Polymer',.55)
    for z in [3,5.5,8,10.5,13,15.5]:box('Recoil pad grooves',(-41.94,0,z),(.1,6.8,.25),'M_C04_Parkerized',.045)
    for side in (-1,1):
        for i in range(6):box('Stock wrist checkering',(-5.2-i*.72,side*3.42,3.0+i*.65),(.22,.12,3),'M_C04_Parkerized',.04,rot=(0,-.5,0))
        for x in (-3.5,11):cyl('870 trigger plate pin',(x,side*3.45,9.5),.43,.17,'M_C04_EdgeSteel','Y',16,.03)
    for a,b in [((-2,0,6),(1,0,.9)),((1,0,.9),(7.5,0,.9)),((7.5,0,.9),(9,0,6))]:beam('870 trigger guard',a,b,.57,armor)
    beam('870 trigger',(4.5,0,5.9),(3.9,0,2.1),.35,'M_C04_EdgeSteel')
    cyl('Cross-bolt safety',(-.2,0,5.8),.43,5.2,'M_C04_EdgeSteel','Y',16)
    box('Action release',(-3,-2.9,5.7),(2.5,.3,.55),'M_C04_EdgeSteel',.1)
    box('Front bead ramp',(59,0,15.7),(3,1.2,.8),armor,.18)
    cyl('Front bead',(60,0,16.4),.24,.38,'M_C04_Ivory','Z',16,.03)
    if upgraded:
        for side in (-1,1):
            profile('Ember receiver shield',[(-6.7,15.5),(11.8,15.5),(13,16.4),(-6.0,16.4)],.15,'M_C04_EmberEnergy',.04,y=side*3.51)
            for x in [-4,-2,0]:box('Ember charge cells',(x,side*3.55,11.5),(1.0,.20,1.3),'M_C04_EmberEnergy',.10)
            box('Stock ember seam',(-27,side*3.52,13.1),(11,.13,.35),'M_C04_EmberEnergy',.06)
    body=commit('SM_'+family+'_Body','870 conventional stock and rounded receiver, fixed barrel/tube and side cross-bolt controls')
    # Retain exact accepted support contact and pump travel, new rounded corn-cob ribs.
    cyl('870 fore-end core',(18,0,6),3.65,17.2,'M_C04_Walnut' if not upgraded else 'M_C04_Polymer',verts=28,bevel=.6)
    for i in range(11):cyl('870 corn-cob rib',(10.1+i*1.58,0,6),3.88,.59,armor,verts=28,bevel=.15)
    for side in (-1,1):
        box('Fore-end palm inset',(18,side*3.90,6.0),(6.8,.14,2.1),'M_C04_Polymer',.25)
        if upgraded:box('Moving ember fore-end inlay',(18,side*4.02,7.1),(5.0,.12,.30),'M_C04_EmberEnergy',.045)
    for y in (-2.8,2.8):beam('Twin action bar',(-1,y,7.5),(25,y,7.5),.28,'M_C04_EdgeSteel')
    pump=commit('SM_'+family+'_ForeEnd','Separate rounded ribbed fore-end and twin action bars; full travel -9X, same gun origin')
    ASSEMBLIES[family]=[body.name,pump.name]

for upgraded in (False,True):pistol(upgraded);rifle(upgraded);shotgun(upgraded)
# Short straight-walled, open-mouth .45-size spent brass; no projectile.
hollow_tube('Spent pistol brass',-1.05,1.15,0,.59,.085,'M_C04_Brass')
cyl('Pistol case head',(-1.07,0,0),.59,.18,'M_C04_Brass',verts=28,bevel=.02)
cyl('Extractor rim',(-1.20,0,0),.61,.10,'M_C04_Brass',verts=28,bevel=.015)
cyl('Primer',(-1.258,0,0),.20,.025,'M_C04_EdgeSteel',verts=20,bevel=.005)
commit('SM_M1911_Case','Spent straight-walled pistol brass with extractor rim and open mouth; centred +X case axis')

# A clean, editable assembly catalogue. Original separate parts remain available.
all_meshes={o.name:o for o in bpy.data.objects if o.type=='MESH'}
for i,(family,names) in enumerate(ASSEMBLIES.items()):
    empty=bpy.data.objects.new('Assembly_'+family,None);scene.collection.objects.link(empty)
    empty.location=(0,(i%3)*44,(i//3)*38)
    for name in names:all_meshes[name].parent=empty
case=all_meshes['SM_M1911_Case'];case.location=(8,-15,-2)

def studio(target, location, scale, filename, width=1600, height=1000):
    for obj in list(bpy.data.objects):
        if obj.type in {'CAMERA','LIGHT'}:bpy.data.objects.remove(obj,do_unlink=True)
    world=bpy.data.worlds.new('Original neutral source-review studio');world.use_nodes=True
    world.node_tree.nodes['Background'].inputs[0].default_value=(.11,.14,.17,1)
    world.node_tree.nodes['Background'].inputs[1].default_value=.65;scene.world=world
    for name,pos,energy,size in [('Key',(100,-170,200),400000,120),('Fill',(-30,180,150),350000,110),('Edge',(-120,40,140),400000,90)]:
        data=bpy.data.lights.new(name,'AREA');data.energy=energy;data.shape='DISK';data.size=size
        obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=Vector(target)+Vector(pos)
        obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    data=bpy.data.cameras.new('Source review');camera=bpy.data.objects.new('Source review',data);scene.collection.objects.link(camera)
    camera.location=location;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
    data.type='ORTHO';data.ortho_scale=scale;data.clip_end=3000;scene.camera=camera
    scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=width;scene.render.resolution_y=height;scene.render.resolution_percentage=100
    scene.view_settings.view_transform='AgX';scene.render.image_settings.file_format='PNG'
    scene.render.filepath=str(SOURCE/filename)
    if '--no-render' not in sys.argv:bpy.ops.render.render(write_still=True)

studio((7,43,20),(150,-205,190),180,'Blender_WeaponCatalogue.png',1600,1150)
scene.render.filepath='//Blender_WeaponCatalogue.png'
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'WeaponWorkshop.blend'))

# Append accepted character+rig only; leave the accepted sources/actions untouched.
with bpy.data.libraries.load(str(ROOT/'ArtSource/Characters/Response.blend'),link=False) as (available,data):
    data.objects=['Rig_Response','SK_Response']
rig,character=data.objects
for obj in data.objects:scene.collection.objects.link(obj);obj.hide_set(False);obj.hide_render=False
rig.location=(0,0,0);rig.animation_data_clear()
for obj in list(bpy.data.objects):
    if obj.name.startswith('Assembly_'):
        for child in obj.children:child.parent=None;child.location=(0,0,0)
        bpy.data.objects.remove(obj,do_unlink=True)
case.hide_render=True;case.hide_set(True)

def zero():
    for bone in rig.pose.bones:
        bone.location=(0,0,0);bone.rotation_mode='QUATERNION';bone.rotation_quaternion=(1,0,0,0);bone.scale=(1,1,1)
    bpy.context.view_layer.update()

def rotate(name,axis,angle):
    bpy.context.view_layer.update();bone=rig.pose.bones[name];m=bone.matrix.copy();p=m.translation.copy()
    bone.matrix=Matrix.Translation(p)@Quaternion(Vector(axis),angle).to_matrix().to_4x4()@Matrix.Translation(-p)@m
    bpy.context.view_layer.update()

def segment(name,start,end):
    rest=rig.data.bones[name];q=(rest.tail_local-rest.head_local).normalized().rotation_difference((Vector(end)-Vector(start)).normalized())
    matrix=q.to_matrix().to_4x4()@rest.matrix_local.to_3x3().to_4x4();matrix.translation=Vector(start)
    rig.pose.bones[name].matrix=matrix;bpy.context.view_layer.update()

def solve(side,goal):
    bpy.context.view_layer.update();upper='upperarm_'+side;lower='lowerarm_'+side
    start=rig.pose.bones[upper].head.copy();goal=Vector(goal);delta=goal-start;direction=delta.normalized()
    a=rig.data.bones[upper].length;b=rig.data.bones[lower].length
    if delta.length>a+b-.05:goal=start+direction*(a+b-.05);delta=goal-start
    distance=max(.1,delta.length);along=(a*a-b*b+distance*distance)/(2*distance)
    height=math.sqrt(max(0,a*a-along*along));pole=Vector((8,-39 if side=='l' else 39,112))
    bend=(pole-start)-direction*(pole-start).dot(direction);joint=start+direction*along+bend.normalized()*height
    segment(upper,start,joint);segment(lower,joint,goal);return goal

def weapon_frame():
    bpy.context.view_layer.update()
    return rig.pose.bones['weapon_r'].matrix@rig.data.bones['weapon_r'].matrix_local.to_3x3().inverted().to_4x4()

def grip(goal,rotation=(0,0,0)):
    wrist=solve('r',goal)
    q=Quaternion(Vector((0,0,1)),rotation[2])@Quaternion(Vector((0,1,0)),rotation[1])@Quaternion(Vector((1,0,0)),rotation[0])
    matrix=q.to_matrix().to_4x4()@rig.data.bones['hand_r'].matrix_local.to_3x3().to_4x4();matrix.translation=wrist
    rig.pose.bones['hand_r'].matrix=matrix;bpy.context.view_layer.update()

def support(offset,hand_yaw_degrees=0):
    frame=weapon_frame();goal=solve('l',frame@Vector(offset))
    matrix=frame.to_3x3().to_4x4()@Quaternion(Vector((0,0,1)),math.radians(hand_yaw_degrees)).to_matrix().to_4x4()@rig.data.bones['hand_l'].matrix_local.to_3x3().to_4x4()
    matrix.translation=goal;rig.pose.bones['hand_l'].matrix=matrix;bpy.context.view_layer.update()

def smooth(x):x=max(0,min(1,x));return x*x*(3-2*x)
def keyed(t,points):
    if t<=points[0][0]:return points[0][1]
    if t>=points[-1][0]:return points[-1][1]
    for (a,va),(b,vb) in zip(points,points[1:]):
        if a<=t<=b:return va+(vb-va)*smooth((t-a)/(b-a))

bone_anchor=bpy.data.objects.new('Evaluated weapon_r',None);scene.collection.objects.link(bone_anchor)
constraint=bone_anchor.constraints.new('COPY_TRANSFORMS');constraint.target=rig;constraint.subtarget='weapon_r'
gun_frame=bpy.data.objects.new('Bind-inverse gun frame',None);scene.collection.objects.link(gun_frame);gun_frame.parent=bone_anchor
gun_frame.matrix_basis=rig.data.bones['weapon_r'].matrix_local.to_3x3().inverted().to_4x4()
hand_anchor=bpy.data.objects.new('Evaluated hand_l',None);scene.collection.objects.link(hand_anchor)
constraint=hand_anchor.constraints.new('COPY_TRANSFORMS');constraint.target=rig;constraint.subtarget='hand_l'
hand_frame=bpy.data.objects.new('Bind-inverse support frame',None);scene.collection.objects.link(hand_frame);hand_frame.parent=hand_anchor
hand_frame.matrix_basis=rig.data.bones['hand_l'].matrix_local.to_3x3().inverted().to_4x4()

def driver(obj,path,expression,prop,index=None):
    d=(obj.driver_add(path) if index is None else obj.driver_add(path,index)).driver;d.expression=expression
    v=d.variables.new();v.name='v';v.type='SINGLE_PROP';v.targets[0].id=rig;v.targets[0].data_path='["'+prop+'"]'

for family,names in ASSEMBLIES.items():
    for name in names:
        obj=all_meshes[name];obj.parent=gun_frame;obj.matrix_parent_inverse=Matrix.Identity(4);obj.matrix_basis=Matrix.Identity(4)
        family_index=['M1911','M4A1','Remington870','LastWord','Overcurrent','Gravebreaker'].index(family)
        driver(obj,'hide_render','abs(v-'+str(family_index)+')>.1','preview_family')
        if name.endswith('_Slide'):driver(obj,'location','-3*v','slide_travel',0)
        if name.endswith('_ForeEnd'):driver(obj,'location','-9*v','pump_travel',0)
        if name.endswith('_Magazine'):
            # Separate handled prop, the released old magazine never teleports back.
            held=obj.copy();held.data=obj.data.copy();scene.collection.objects.link(held);held.name=name+'_FreshPreview'
            held.animation_data_clear();held.parent=hand_frame;held.matrix_basis=Matrix.Translation((.5,0,1.8) if 'M1911' in name or 'LastWord' in name else (-10.5,0,0))
            driver(held,'hide_render','v<.5','fresh_visible')
            # Additional family gating uses a dedicated combined preview property.
            held['family_index']=family_index

ANIMATIONS={};POSE_CHECKS={}
DEFS=[('PistolReady',2.4,'pistol_ready'),('PistolFire',.18,'pistol_fire'),('PistolReload',1.8,'pistol_reload'),('PistolEquip',.36,'pistol_equip'),
      ('CarbineReload',2.10,'carbine_reload'),('UnarmedReady',2.4,'unarmed')]
for kind in ['Pistol','Carbine','Shotgun']:
    DEFS.extend([(kind+'Handoff',.72,kind.lower()+'_handoff'),(kind+'Retrieve',.64,kind.lower()+'_retrieve')])

def pose_action(name,duration,kind):
    action=bpy.data.actions.new('A_Response_C04_'+name);action.use_fake_user=True;rig.animation_data_create();rig.animation_data.action=action
    scene.frame_start=1;scene.frame_end=round(duration*FPS)+1
    samples=[]
    for index in range(round(duration*FPS)+1):
        t=index/FPS;scene.frame_set(index+1);zero()
        pistol_pose=kind.startswith('pistol');is_carbine=kind.startswith('carbine')
        family=0 if pistol_pose else (1 if is_carbine else 2)
        rig['preview_family']=float(family);rig['slide_travel']=0.;rig['pump_travel']=0.;rig['fresh_visible']=0.;rig['seated_visible']=1.
        ready=Vector((42,4,138)) if pistol_pose else Vector((24,5,131))
        ready_support=Vector((-1.0,-4.0,-.4)) if pistol_pose else Vector((18,0,6))
        goal=ready.copy();rotation=Vector((0,0,0));support_goal=ready_support.copy()
        if kind=='pistol_ready':
            goal.z+=.10*math.sin(t/duration*math.tau)
        elif kind=='pistol_fire':
            kick=keyed(t,[(0,0),(.025,1),(.07,.5),(.13,.12),(.18,0)])
            goal.x-=1.5*kick;goal.z+=.7*kick;rotation.y=math.radians(-8*kick)
            rig['slide_travel']=keyed(t,[(0,0),(.025,1),(.07,0),(.18,0)])
        elif kind=='pistol_reload':
            amount=keyed(t,[(0,0),(.22,1),(1.43,1),(1.8,0)])
            goal=ready.lerp(Vector((29,8,125)),amount);rotation.x=math.radians(-25*amount);rotation.y=math.radians(-12*amount)
            support_goal=keyed(t,[(0,ready_support),(.18,Vector((-.5,0,-5.6))),(.28,Vector((-.5,0,-7.7))),(.51,Vector((-14,-9,-17))),
                (.64,Vector((-12,-8,-16))),(.94,Vector((-.5,0,-6.5))),(1.10,Vector((-.5,0,-1.8))),(1.25,Vector((-1,-5,1.5))),
                (1.40,Vector((-.6,-1.4,2.3))),(1.58,Vector((-1,-4,1))),(1.8,ready_support)])
            rig['fresh_visible']=float(.64<=t<1.10);rig['seated_visible']=float(t<.28 or t>=1.10)
            rig['slide_travel']=1. if t<1.40 else 1-smooth((t-1.40)/.06)
        elif kind=='pistol_equip':
            amount=max(0.,math.sin(math.pi*t/duration))**1.4;goal.z-=25*amount;goal.x-=12*amount;rotation.y=math.radians(42*amount)
        elif kind=='carbine_reload':
            amount=keyed(t,[(0,0),(.22,1),(1.80,1),(2.1,0)])
            goal=ready.lerp(Vector((24,7,127)),amount);rotation.x=math.radians(-10*amount)
            support_goal=keyed(t,[(0,ready_support),(.28,Vector((10.5,0,0))),(.40,Vector((10.5,0,-3))),(.65,Vector((-10,-10,-24))),
                (.74,Vector((-10,-10,-24))),(1.08,Vector((10.5,0,-9))),(1.20,Vector((10.5,0,0))),(1.46,Vector((13.5,0,1))),
                (1.68,Vector((4.5,4,13))),(1.74,Vector((1.5,4,13))),(1.88,Vector((8,0,9))),(2.10,ready_support)])
            rig['fresh_visible']=float(.74<=t<1.20);rig['seated_visible']=float(t<.40 or t>=1.20)
        elif kind.endswith('_handoff'):
            extension=keyed(t,[(0,0),(.18,.20),(.48,1),(.54,1),(.72,.65)])
            goal=ready.lerp(Vector((48,3,119)) if pistol_pose else Vector((38,4,119)),extension)
            rotation.y=math.radians(3*extension)
            if t>.48:support_goal=ready_support+Vector((-6,-4,-5))*smooth((t-.48)/.24)
        elif kind.endswith('_retrieve'):
            extension=keyed(t,[(0,.55),(.18,1),(.28,1),(.64,0)])
            goal=ready.lerp(Vector((48,3,119)) if pistol_pose else Vector((38,4,119)),extension)
            rotation.y=math.radians(3*extension)
        elif kind=='unarmed':
            goal=Vector((6,19,100));support_goal=Vector((0,0,0));rig['preview_family']=-1.
        grip(goal,rotation)
        if kind=='unarmed':
            wrist=solve('l',(6,-19,100));matrix=rig.data.bones['hand_l'].matrix_local.copy();matrix.translation=wrist;rig.pose.bones['hand_l'].matrix=matrix
        else:
            hand_yaw=20. if pistol_pose else 0.
            if kind=='pistol_reload':hand_yaw=keyed(t,[(0,20.),(.18,0.),(1.43,0.),(1.80,20.)])
            support(support_goal,hand_yaw)
        for bone in rig.pose.bones:
            for path in ['location','rotation_quaternion','scale']:bone.keyframe_insert(path,frame=index+1,group=bone.name)
        for prop in ['preview_family','slide_travel','pump_travel','fresh_visible','seated_visible']:rig.keyframe_insert('["'+prop+'"]',frame=index+1)
        if index in {0,round(duration*FPS),round(duration*FPS/2),28,40,48,64,110,120,140}:
            samples.append({'seconds':t,'weapon_origin_source_cm':list(weapon_frame().translation),'support_wrist_source_cm':list(rig.pose.bones['hand_l'].head),
                           'support_target_error_cm':0 if kind=='unarmed' else (rig.pose.bones['hand_l'].head-(weapon_frame()@support_goal)).length})
    scene.frame_set(1);export_fbx(EXPORT/(action.name+'.fbx'),[rig],True)
    ANIMATIONS[action.name]={'source':f'ArtSource/Exports/Candidate04/{action.name}.fbx','asset':f'/Game/ONE/Animations/Candidate04/{action.name}',
        'duration':duration,'kind':kind,'layer':'spine_01 upper body; no C03 locomotion modification'}
    POSE_CHECKS[action.name]=samples
    return action

actions={name:pose_action(name,duration,kind) for name,duration,kind in DEFS}
# Family-aware visibility and coherent seated/fresh props in the editable source.
for obj in bpy.data.objects:
    if obj.name.endswith('_FreshPreview'):
        d=obj.driver_add('hide_render').driver;d.expression='a<.5 or abs(b-'+str(obj['family_index'])+')>.1'
        while d.variables:d.variables.remove(d.variables[0])
        for name,prop in [('a','fresh_visible'),('b','preview_family')]:
            v=d.variables.new();v.name=name;v.type='SINGLE_PROP';v.targets[0].id=rig;v.targets[0].data_path='["'+prop+'"]'
    elif obj.name.endswith('_Magazine') and obj.name in all_meshes:
        family_index=next(i for i,f in enumerate(['M1911','M4A1','Remington870','LastWord','Overcurrent','Gravebreaker']) if obj.name in ASSEMBLIES[f])
        d=obj.driver_add('hide_render').driver;d.expression='a<.5 or abs(b-'+str(family_index)+')>.1'
        while d.variables:d.variables.remove(d.variables[0])
        for name,prop in [('a','seated_visible'),('b','preview_family')]:
            v=d.variables.new();v.name=name;v.type='SINGLE_PROP';v.targets[0].id=rig;v.targets[0].data_path='["'+prop+'"]'

for action_name,frame,filename in [('PistolReady',1,'Blender_PistolReady.png'),('PistolReload',111,'Blender_PistolMagazineSeat.png'),('CarbineHandoff',49,'Blender_CarbineHandoff.png')]:
    rig.animation_data.action=actions[action_name];scene.frame_set(frame);bpy.context.view_layer.update()
    studio((28,0,132),(180,-235,215),107,filename,1200,1000)
rig.animation_data.action=actions['PistolReady'];scene.frame_start=1;scene.frame_end=241;scene.frame_set(1)
scene.render.filepath='//Blender_PistolReady.png'
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/'ResponseWeaponActions.blend'))

inventory={'candidate':'04','generator':'Scripts/create_candidate04_weapon_assets.py','importer':'Scripts/import_candidate04_weapon_assets.py',
    'provenance':'Original Project ONE geometry and authored rig actions. C04 refines Project ONE grip/fore-end interfaces; no third-party asset, texture, animation or recording input.',
    'reference_pages':[{'url':'https://www.colt.com/wp-content/uploads/2023/02/wwiireproductionpistolmodelm1911a1.pdf','use':'M1911A1 silhouette, fixed sights, 5-inch barrel proportions and seven-round reference; original geometry, no copied logo'},
        {'url':'https://www.colt.com/wp-content/uploads/2024/08/2024-Colt-MLE-Catalogweb.pdf','use':'M4 carbine page24: stock, short handguard, A-frame sight and 14.5-inch barrel identity; gameplay ammunition remains independent'},
        {'url':'https://www.remarms.com/shotguns/pump-action/model-870/','use':'870 conventional stock, rounded receiver, tube magazine and pump family visual identity'}],
    'blend_sources':['ArtSource/Weapons/Candidate04/WeaponWorkshop.blend','ArtSource/Weapons/Candidate04/ResponseWeaponActions.blend'],
    'coordinate_convention':'Centimetres, +X barrel, +Z up. Every assembled static part shares the grip origin. Legacy FBX import reflects source Y; offsets below are already UE coordinates.',
    'fps':FPS,'skeleton':'/Game/ONE/Characters/SK_Response_Skeleton','static_meshes':STATIC,'assemblies':ASSEMBLIES,'animations':ANIMATIONS,
    'materials':{k:{'base_color':v[0],'roughness':v[1],'metallic':v[2],'emissive_gain':v[3]} for k,v in PALETTE.items()},
    'runtime':{'weapon_attachment':'weapon_r with component-space reference quaternion inverse, as accepted C03; no extra scale or arbitrary corrective yaw',
        'M1911':{'muzzle_ue_cm':[16.5,0,5.5],'ejection_ue_cm':[5.7,-1.5,5.5],'slide_travel_cm':[-3,0,0],
            'slide_fire_curve':[[0,0],[.025,1],[.07,0],[.18,0]],'held_magazine_translation_gun_axes':[.5,0,1.8],
            'reload':{'duration':1.8,'mag_out_drop':.28,'fresh_visible':.64,'mag_commit':1.10,'slide_release':1.40,'ready':1.8}},
        'M4A1':{'muzzle_ue_cm':[54.5,0,14],'ejection_ue_cm':[3,-3.7,14],'held_magazine_translation_gun_axes':[-10.5,0,0],
            'reload':{'duration':2.1,'mag_out_drop':.40,'fresh_visible':.74,'mag_commit':1.20,'bolt':1.74,'ready':2.1}},
        'Remington870':{'muzzle_ue_cm':[64.5,0,14],'ejection_ue_cm':[5,-4.5,13.5],'fore_end_center_cm':[18,0,6],
            'pump_travel_cm':[-9,0,0],'pump_curve_seconds':[[0,0],[.21,1],[.44,0],[.56,0]],'pump_eject_seconds':.18,'reuse_C02_fire_pump_shell_clips':True},
        'handoff':{'duration':.72,'transfer_seconds':.48,'retrieval_duration':.64,'retrieval_take_seconds':.18,
            'alignment':'Sample evaluated gun transform at transfer, then interpolate actual assembled preview into the machine cradle. Do not relocate or scale the player/gun.'},
        'upgrade_event_scaling':'Divide fire and pump durations AND related event times by1.15; sample source animation time at operation elapsed*1.15. Do not speed locomotion or reload globally.'},
    'source_pose_checks':POSE_CHECKS,'validation':{'static_count':len(STATIC),'animation_count':len(ANIMATIONS),'authored_bones':len(rig.data.bones),'expected_runtime_bones':22,
        'finite_vertices':True,'old_sources_modified':False,'status':'BLENDER_AUTHORED_NOT_YET_UNREAL_IMPORTED'},
    'limitations':['Blender source renders are not gameplay evidence','Existing glove meshes have no finger bones; fine finger articulation is not newly added',
        'Original weapon models use selective geometry/vertex materials, no texture scans or manufacturer logos','Source action checks do not establish evaluated gameplay contacts or perceptual audio quality']}
(SOURCE/'inventory.json').write_text(json.dumps(inventory,indent=2)+'\n',encoding='utf-8')
print('C04 WEAPON SOURCE COMPLETE',len(STATIC),'meshes',len(ANIMATIONS),'actions')
