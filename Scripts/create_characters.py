"""Project ONE original character workshop. Blender 5.1 background compatible.

Run: blender --background --python Scripts/create_characters.py
All dimensions are centimetres, X forward, Y right, Z up. No external asset inputs.
"""
from pathlib import Path
import bpy, math, json, sys
from mathutils import Vector, Matrix, Quaternion

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource' / 'Exports'
SRC = ROOT / 'ArtSource' / 'Characters'
OUT.mkdir(parents=True, exist_ok=True)
SRC.mkdir(parents=True, exist_ok=True)
FPS = 30
HEAD_ONLY = '--head-only' in sys.argv
BODY_ONLY = '--body-only' in sys.argv
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = .01
bpy.context.scene.render.fps = FPS

def mat(name, color, roughness=.65, metal=0):
    m=bpy.data.materials.new(name); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value=roughness
    p.inputs['Metallic'].default_value=metal
    m.diffuse_color=(*color,1)
    return m

M={
 'uniform':mat('M_UniformSlate',(.105,.145,.165),.86),
 'plate':mat('M_Ceramic',(.19,.235,.245),.68,.12),
 'web':mat('M_Webbing',(.04,.055,.06),.88),
 'rubber':mat('M_BootRubber',(.018,.022,.022),.77),
 'skin':mat('M_Skin',(.48,.30,.215),.68),
 'hair':mat('M_Hair',(.035,.025,.022),.85),
 'orange':mat('M_OrangeFabric',(.36,.145,.055),.81),
 'infected':mat('M_InfectedSkin',(.48,.445,.355),.73),
 'gore':mat('M_Gore',(.23,.014,.018),.47),
 'bone':mat('M_ExposedBone',(.55,.48,.33),.74),
 'badge':mat('M_IDBadge',(.76,.76,.65),.63),
 'visor':mat('M_Visor',(.05,.135,.16),.18,.5),
 'reflective':mat('M_ReflectiveTape',(.42,.47,.42),.42,.22),
 'eye':mat('M_Eye',(.33,.32,.22),.33),
 'pupil':mat('M_EyeDark',(.012,.017,.016),.33),
 'wound':mat('M_DriedBlood',(.105,.018,.015),.85),
 'accent':mat('M_ResponseAccent',(.60,.34,.085),.45),
}

def mesh(name,verts,faces,material,weights=None):
    data=bpy.data.meshes.new(name+'_Mesh'); data.from_pydata(verts,[],faces); data.update()
    o=bpy.data.objects.new(name,data); bpy.context.collection.objects.link(o)
    o.data.materials.append(M[material])
    for f in data.polygons: f.use_smooth=True
    if weights:
        for i, w in enumerate(weights):
            for bone, amount in w.items():
                vg=o.vertex_groups.get(bone) or o.vertex_groups.new(name=bone)
                if amount>.00001: vg.add([i],amount,'REPLACE')
    return o

def weight(bone): return lambda p: {bone:1.0}
def torso_weights(p):
    z=p[2]
    if z<102: return {'pelvis':1}
    if z<116:
        f=(z-102)/14; return {'pelvis':1-f,'spine_01':f}
    if z<131:
        f=(z-116)/15; return {'spine_01':1-f,'spine_02':f}
    return {'spine_02':1}

def loft(name, rings, material, wf, sides=24, cap=True, wrinkle=0):
    """Closed tailored horizontal ring surface; ring=(z,cx,cy,depth,width)."""
    verts=[]; weights=[]
    for j,(z,cx,cy,rx,ry) in enumerate(rings):
        for k in range(sides):
            a=2*math.pi*k/sides
            ripple=1+wrinkle*math.sin(3*a+j*1.7)*math.sin(math.pi*j/max(1,len(rings)-1))
            p=(cx+rx*math.cos(a)*ripple,cy+ry*math.sin(a)*ripple,z)
            verts.append(p); weights.append(wf(p))
    faces=[]
    for j in range(len(rings)-1):
        for k in range(sides):
            a=j*sides+k; b=j*sides+(k+1)%sides
            faces.append((a,b,b+sides,a+sides))
    if cap:
        faces.append(tuple(reversed(range(sides))))
        faces.append(tuple((len(rings)-1)*sides+k for k in range(sides)))
    return mesh(name,verts,faces,material,weights)

def tube(name, start, end, profile, material, bone, other=None, sides=20, cap=True):
    """Contoured limb; actual connected radial mesh with joint blending."""
    start=Vector(start); end=Vector(end); d=(end-start).normalized()
    ref=Vector((1,0,0)) if abs(d.x)<.9 else Vector((0,1,0))
    u=(ref-d*ref.dot(d)).normalized(); v=d.cross(u).normalized()
    verts=[]; weights=[]
    cloth=material in {'uniform','orange'} and 'stump' not in name.lower() and 'shoulder' not in name.lower()
    if cloth:
        dense=[]
        for j in range(33):
            f=j/32
            for i in range(len(profile)-1):
                if profile[i][0]<=f<=profile[i+1][0]:
                    mix=(f-profile[i][0])/(profile[i+1][0]-profile[i][0])
                    dense.append((f,profile[i][1]*(1-mix)+profile[i+1][1]*mix,profile[i][2]*(1-mix)+profile[i+1][2]*mix)); break
        profile=dense
    for f,r1,r2 in profile:
        c=start.lerp(end,f)
        for k in range(sides):
            a=2*math.pi*k/sides
            fold=0
            if cloth:
                envelope=math.exp(-((f-.84)/.13)**2)+.3*math.exp(-((f-.14)/.13)**2)
                boundary=min(1,f/.055,(1-f)/.055)
                directional=.3+.7*(.5+.5*math.cos(a-.7))**2
                fold=.58*envelope*boundary*directional*math.sin(f*math.pi*12+3.5*math.sin(a)+1.3*math.sin(a*3))
                if 'Trouser_lower' in name and c.z<28: fold=0
            p=c+u*(math.cos(a)*(r1+fold))+v*(math.sin(a)*(r2+fold))
            verts.append(p)
            blend=max(0,(f-.74)/.26)*.45 if other else 0
            weights.append({bone:1-blend,**({other:blend} if other else {})})
    faces=[]
    for j in range(len(profile)-1):
        for k in range(sides):
            a=j*sides+k; b=j*sides+(k+1)%sides
            faces.append((a,b,b+sides,a+sides))
    if cap:
        faces.append(tuple(reversed(range(sides))))
        faces.append(tuple((len(profile)-1)*sides+k for k in range(sides)))
    result=mesh(name,verts,faces,material,weights)
    if cap:
        result.data.polygons[-1].use_smooth=False
        result.data.polygons[-2].use_smooth=False
    return result

def bevelbox(name, loc, scale, material, bone, bevel=1, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=2,location=loc)
    o=bpy.context.object; o.name=name; o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if rot: o.rotation_euler=rot
    o.data.materials.append(M[material])
    mod=o.modifiers.new('Sewn corners','BEVEL'); mod.width=bevel; mod.segments=3
    bpy.context.view_layer.objects.active=o; bpy.ops.object.modifier_apply(modifier=mod.name)
    for f in o.data.polygons: f.use_smooth=True
    vg=o.vertex_groups.new(name=bone); vg.add(list(range(len(o.data.vertices))),1,'REPLACE')
    return o

def ellipsoid(name,loc,scale,material,bone,segments=24,rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,location=loc)
    o=bpy.context.object; o.name=name; o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(M[material])
    vg=o.vertex_groups.new(name=bone); vg.add(list(range(len(o.data.vertices))),1,'REPLACE')
    for f in o.data.polygons: f.use_smooth=True
    return o

def strap(name,points,width,thick,material,bone):
    verts=[]
    for i,p in enumerate(points):
        d=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
        across=d.normalized().cross(Vector((1,0,0))).normalized()*width/2
        if across.length<.1: across=Vector((0,width/2,0))
        for x in [-thick/2,thick/2]:
            for s in [-1,1]: verts.append(Vector(p)+Vector((x,0,0))+across*s)
    faces=[]
    for i in range(len(points)-1):
        a=i*4; b=a+4
        faces.extend([(a,b,b+1,a+1),(a+2,a+3,b+3,b+2),(a,a+2,b+2,b),(a+1,b+1,b+3,a+3)])
    faces.extend([(0,1,3,2),(len(verts)-4,len(verts)-2,len(verts)-1,len(verts)-3)])
    return mesh(name,verts,faces,material,[{bone:1}]*len(verts))

REST={
 'root':((0,0,0),(0,0,15),None),
 'pelvis':((0,0,95),(0,0,108),'root'),
 'spine_01':((0,0,108),(0,0,127),'pelvis'),
 'spine_02':((0,0,127),(0,0,148),'spine_01'),
 'neck':((0,0,148),(0,0,157),'spine_02'),
 'head':((0,0,157),(0,0,176),'neck'),
}
for side,sgn in [('l',-1),('r',1)]:
    REST['thigh_'+side]=((0,sgn*9,95),(1.2,sgn*9,53),'pelvis')
    REST['calf_'+side]=((1.2,sgn*9,53),(0,sgn*9,10),'thigh_'+side)
    REST['foot_'+side]=((0,sgn*9,10),(16,sgn*9,7),'calf_'+side)
    REST['toe_'+side]=((16,sgn*9,7),(22,sgn*9,7),'foot_'+side)

def rest_for(kind):
    r=dict(REST)
    for side,sgn in [('l',-1),('r',1)]:
        shoulder=(0,sgn*18,145)
        if kind=='Response':
            elbow=(18,-15,120) if side=='l' else (6,25,119)
            wrist=(42,5,137) if side=='l' else (24,5,131)
            fingertips=(wrist[0]+8,wrist[1],wrist[2])
        else:
            elbow=(4,sgn*26,117)
            wrist=(9,sgn*27,92)
            fingertips=(12,sgn*27,82)
        r['upperarm_'+side]=(shoulder,elbow,'spine_02')
        r['lowerarm_'+side]=(elbow,wrist,'upperarm_'+side)
        r['hand_'+side]=(wrist,fingertips,'lowerarm_'+side)
    r['weapon_r']=((24,5,131),(44,5,131),'hand_r')
    return r

def make_rig(kind):
    bones=rest_for(kind)
    arm=bpy.data.armatures.new('Rig_'+kind)
    rig=bpy.data.objects.new('Rig_'+kind,arm); bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active=rig; rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for name,(h,t,parent) in bones.items():
        b=arm.edit_bones.new(name); b.head=h; b.tail=t
        if parent: b.parent=arm.edit_bones[parent]
        b.use_deform=name!='weapon_r'
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.show_in_front=True; arm.display_type='OCTAHEDRAL'
    rig.select_set(False)
    return rig,bones

def face_parts(kind):
    infected=kind=='Infected'; skin='infected' if infected else 'skin'
    p=[]
    # Head shape includes brow ridge, cheek planes, narrower jaw and full rear skull.
    rings=[(157.8,0,0,4.0,4.3),(160,0,0,4.6,4.5),(161.5,.4,0,5.2,5.5),
           (163.5,.1,0,6.4,6.4),(166,-.2,0,7.0,7.3),(168,-.6,0,7.4,7.8),
           (170,-.7,0,7.65,7.9),(172,-.8,0,7.65,7.9),(174,-1,0,7.3,7.6),
           (176,-1.1,0,6.35,6.4),(178,-1.2,0,4.3,4.5),(179,-1.2,0,.5,.6)]
    skull=loft('Anatomical_skull_'+kind,rings,skin,weight('head'),64)
    # Connected sculpted face planes; detail is part of the head surface.
    for vert in skull.data.vertices:
        x,y,z=vert.co
        if x>0:
            front=max(0,min(1,x/5))
            gauss=lambda c,w: math.exp(-((z-c)/w)**2)
            nose=(2.5*gauss(168,1.6)+.7*gauss(171,2))*math.exp(-(y/1.1)**2)
            orbit=-.65*gauss(170.6,.85)*math.exp(-((abs(y)-3)/1.4)**2)
            cheek=.6*gauss(167.7,1.5)*math.exp(-((abs(y)-4.8)/1.6)**2)
            lip=.38*gauss(165,.65)*math.exp(-(y/2.1)**4)
            vert.co.x+=front*(nose+orbit+cheek+lip)
        if infected:
            # A slightly broader asymmetric cranium retains a human head silhouette
            # when the face is foreshortened by the gameplay camera.
            if z>161: vert.co.y*=1.07
            if z>174: vert.co.z+=.55*math.sin(min(1,(z-174)/5)*math.pi/2)
    p.append(skull)
    for s in [-1,1]:
        p.append(ellipsoid('Ear',(0,s*7.8,168.9),(1.5,1.1,2.55),skin,'head'))
        p.append(ellipsoid('Ear_fold',(.8,s*8.1,169),(1,.3,1.5),'wound' if infected else 'skin','head'))
        p.append(ellipsoid('Orbital_recess',(5.78,s*3.0,170.6),(.22,1.7,.64),'wound' if infected else 'hair','head'))
        p.append(ellipsoid('Eye',(6.0,s*2.9,170.7),(.22,1.08,.37),'eye','head'))
        p.append(ellipsoid('Pupil',(6.22,s*2.9,170.7),(.055,.26,.26),'pupil','head'))
    p.append(ellipsoid('Lower_lip',(6.6,0,164.7),(.6,2.5,.52),'wound' if infected else 'skin','head'))
    p.append(ellipsoid('Mouth_line',(6.95,0,165.6),(.27,2.35,.43 if infected else .18),'hair','head'))
    if infected:
        # One ragged cheek abrasion follows the cheek rather than raised red spheres.
        p.append(strap('Cheek_abrasion',[(5.8,-4.8,168.5),(6,-4.4,166.8),(5.6,-4,165.9)],1.4,.08,'wound','head'))
        for y in [-1.4,-.45,.45,1.4]:
            p.append(bevelbox('Uneven_teeth',(7.22,y,165.8),(.1,.30,.23),'bone','head',.1))
        # Balding infected worker: side/rear stubble only. A solid black crown read
        # as a severed neck disk in actual gameplay, so the pale scalp stays exposed.
        hair_verts=[tuple(v.co+v.normal*.07) for v in skull.data.vertices]
        hair_faces=[]
        for face in skull.data.polygons:
            center=sum((skull.data.vertices[i].co for i in face.vertices),Vector())/len(face.vertices)
            if 171.5<center.z<175.2 and center.x<.6 and abs(center.y)>3:
                hair_faces.append(tuple(face.vertices))
        p.append(mesh('Temporal_stubble',hair_verts,hair_faces,'hair',[{'head':1}]*len(hair_verts)))
        p.append(strap('Scalp_laceration',[(5.0,-3.8,174.5),(4.0,-3.8,176),(2.2,-3.0,178.5)],1.3,.07,'gore','head'))
        p.append(tube('Neck_sever_interior',(0,0,157.8),(0,0,159.3),[(0,3.7,4),(1,3.5,3.8)],'gore','head',sides=32))
    else:
        # Faceted modern response helmet and opaque ballistic goggles.
        p.append(loft('Helmet_shell',[(171.7,-1.8,0,8.0,9.1),(173.8,-1.8,0,8.7,9.4),(177,-1.8,0,8.0,8.6),(180,-1.8,0,5.8,6.4),(181.2,-1.8,0,1,1.2)],'plate',weight('head'),40))
        p.append(loft('Helmet_rim',[(171.8,-1.8,0,8.3,9.35),(172.5,-1.8,0,8.4,9.45),(173,-1.8,0,8.2,9.3)],'web',weight('head'),40))
        p.append(bevelbox('Goggle_bridge',(8.2,0,170.8),(.85,6.4,1.85),'web','head',.8))
        for s in [-1,1]:
            p.append(bevelbox('Ballistic_goggle_lens',(9,s*3.25,170.9),(.25,2.6,1.3),'visor','head',.65))
            p.append(strap('Helmet_strap',[(0,s*8.3,172),(2,s*7.3,164),(4,s*3.7,161.2)],1.25,.3,'web','head'))
            p.append(bevelbox('Helmet_side_rail',(-1,s*9.4,174),(3,.5,.8),'web','head',.4))
        p.append(bevelbox('Helmet_identification',(6.8,0,176),(.4,2.7,1.2),'accent','head',.3))
        # Tight half respirator follows the jaw; cartridges remain small and practical.
        p.append(loft('Response_half_respirator',[(161.6,3.9,0,2.6,4.2),(163.5,5.5,0,3.2,5.5),(166,6.0,0,3.1,5.8),(168.1,6.4,0,2.4,3.3)],'web',weight('head'),32))
        p.append(bevelbox('Respirator_front_filter',(9.1,0,164.5),(1.0,3,2.2),'plate','head',.9))
        for y in [-2,-1,0,1,2]: p.append(bevelbox('Respirator_vent',(10.1,y,164.5),(.12,.18,1.3),'rubber','head',.1))
    return p

def arm_parts(kind,bones,side):
    infected=kind=='Infected'; sgn=-1 if side=='l' else 1
    ub='upperarm_'+side; lb='lowerarm_'+side; hb='hand_'+side
    a,b,_=bones[ub]; _,c,_=bones[lb]; _,d,_=bones[hb]
    a,b,c,d=map(Vector,(a,b,c,d)); p=[]
    start=a+(b-a).normalized()*(4.8 if infected and side=='l' else 0)
    cloth='orange' if infected else 'uniform'
    p.append(tube('Tailored_upper_sleeve_'+side,start,b,[(0,6.7,6.2),(.14,7.5,6.8),(.38,7.0,6.3),(.64,5.8,5.5),(.9,5.1,5.0),(1,4.9,4.8)],cloth,ub,lb,sides=28))
    if infected:
        sleeve_end=b.lerp(c,.43 if side=='l' else .66)
        p.append(tube('Torn_lower_sleeve_'+side,b,sleeve_end,[(0,5.1,4.8),(.25,5.6,4.9),(.58,5.1,4.6),(.85,4.7,4.2),(1,4.3,4.1)],cloth,lb,sides=24))
        p.append(tube('Exposed_forearm_'+side,b.lerp(c,.35),c,[(0,4.7,4.2),(.25,4.5,3.9),(.55,3.7,3.2),(.85,2.8,2.5),(1,2.7,2.4)],'infected',lb,hb,sides=24))
        if side=='l':
            direction=(b-a).normalized()
            p.append(tube('Arm_sever_cap',start,start+direction*.5,[(0,6.65,6.15),(1,6.3,5.9)],'gore',ub,sides=32))
            p.append(tube('Humerus_cross_section',start-direction*.05,start+direction*.8,[(0,1.2,1.2),(1,1.1,1.1)],'bone',ub,sides=20))
    else:
        p.append(tube('Lower_sleeve_'+side,b,c,[(0,5.1,4.8),(.18,5.6,5.0),(.42,5.3,4.6),(.7,4,3.5),(.92,3.2,2.9),(1,3,2.7)],cloth,lb,hb,sides=28))
        p.append(bevelbox('Elbow_pad_'+side,b+Vector((-1,sgn*3.0,0)),(4.1,2.7,3.7),'plate',lb,1.1))
        p.append(tube('Cuff_'+side,c.lerp(b,.14),c,[(0,3.7,3.3),(.4,3.65,3.3),(1,3.6,3.2)],'web',lb,sides=24))
    # Palm and individually jointed curled fingers; gripping a rifle in response pose.
    hd=(d-c).normalized(); palm_end=c+hd*6.5
    p.append(tube('Palm_'+side,c,palm_end,[(0,2.7,2.5),(.2,3.3,2.35),(.7,3.4,2.25),(1,3.0,2.0)],'infected' if infected else 'web',hb,sides=20))
    cross=Vector((0,1,0))
    for i in range(4):
        f0=palm_end+cross*((i-1.5)*1.5)
        f1=f0+hd*(3.5 if i in (1,2) else 2.8)+Vector((-1.2,0,-1.8) if infected else (0,0,-2.8))
        f2=f1+Vector((-1.1,0,-2.2))
        p.append(tube('Finger_%s_%s'%(side,i),f0,f1,[(0,.9,.82),(.5,.85,.78),(1,.73,.7)],'infected' if infected else 'web',hb,sides=10))
        p.append(tube('Fingertip_%s_%s'%(side,i),f1,f2,[(0,.74,.7),(.6,.66,.65),(1,.3,.3)],'infected' if infected else 'web',hb,sides=10))
    thumb0=c+hd*2+cross*(sgn*3)
    p.append(tube('Thumb_'+side,thumb0,thumb0+hd*4-cross*sgn*2,[(0,1.3,1.1),(.5,1.15,1),(1,.65,.65)],'infected' if infected else 'web',hb,sides=12))
    return p

def body_parts(kind,bones):
    infected=kind=='Infected'; p=[]; cloth='orange' if infected else 'uniform'
    # Torso sewn silhouette: tapered waist, lumbar curve, ribcage, shoulders and collar.
    p.append(loft('Coverall_torso' if infected else 'Combat_blouse',[
        (93,-1,0,9.4,14),(99,-.3,0,11,16),(106,-.6,0,10.5,15.7),
        (113,-1,0,10.2,14.8),(120,-.5,0,11,16),(127,.0,0,12.5,18),
        (134,.4,0,12.8,19),(140,0,0,11.4,20),(145,-1,0,8.4,20),
        (148,-1,0,6.5,13.5),(150,-.4,0,5,6.1)],cloth,torso_weights,40,True,.028))
    p.append(loft('Collar',[(147.5,-.3,0,5.5,6),(153,-.3,0,5.2,5.6),(154.7,-.3,0,4.7,5.1)],'web' if not infected else 'orange',weight('neck'),32))
    p.append(loft('Lower_neck',[(149,0,0,4.5,4.5),(153,0,0,4.1,4.2),(157.8,0,0,4,4.3)],'infected' if infected else 'skin',weight('neck'),32))
    if infected:
        p.append(tube('Neck_stump_cap',(0,0,157.7),(0,0,158),[(0,3.95,4.25),(1,3.93,4.23)],'gore','neck',sides=32))
        p.append(tube('Spinal_cross_section',(0,0,157.7),(0,0,158.05),[(0,1.5,1.4),(1,1.5,1.4)],'bone','neck',sides=20))
        a,b,_=bones['upperarm_l']; a=Vector(a); b=Vector(b); cut=a+(b-a).normalized()*4.8
        p.append(tube('Capped_left_shoulder',a,cut,[(0,6.8,6.3),(.3,7.1,6.5),(1,6.7,6.2)],'orange','upperarm_l',sides=32))
        p.append(tube('Shoulder_stump_interior',cut,cut+(b-a).normalized()*.08,[(0,6.65,6.15),(1,6.65,6.15)],'gore','upperarm_l',sides=32))
        zipper=strap('Open_coverall_zipper',[(10.7,0,105),(11,0,116),(12.4,0,125),(12.3,0,136),(6,0,148)],.7,.35,'web','spine_02')
        zipper.vertex_groups.clear()
        for vert in zipper.data.vertices:
            for name,amount in torso_weights(vert.co).items():
                group=zipper.vertex_groups.get(name) or zipper.vertex_groups.new(name=name)
                group.add([vert.index],amount,'REPLACE')
        p.append(zipper)
        p.append(bevelbox('Worker_ID',(12.7,8.3,135),(.25,2.5,3.5),'badge','spine_02',.25))
        p.append(bevelbox('ID_portrait',(13.05,8.3,136),(.06,1.1,1.25),'web','spine_02',.15))
        p.append(bevelbox('ID_barcode',(13.07,8.3,133.7),(.05,1.9,.35),'orange','spine_02',.1))
        for s in [-1,1]:
            p.append(strap('Worker_reflective_chest',[(11.2,s*5.6,139),(10.7,s*11,139),(7,s*16,138)],2.3,.28,'reflective','spine_02'))
        # Contamination is vertex-painted into the fabric below, rather than raised patches.
    else:
        p.append(loft('Plate_carrier_wrap',[(113,0,0,11.5,16),(116,0,0,12.1,16.3),(135,0,0,14,18.3),(140,-.2,0,12,18),(143,-1,0,9,14.5)],'web',torso_weights,32,True))
        p.append(bevelbox('Front_ceramic_plate',(13.1,0,130),(2.5,12.2,11.7),'plate','spine_02',2.4))
        p.append(bevelbox('Rear_ceramic_plate',(-12,0,130),(2.2,12,12),'plate','spine_02',2.5))
        for s in [-1,1]:
            p.append(strap('Carrier_shoulder_strap',[(12,s*10,135),(7,s*12,144),(-3,s*13,149),(-12,s*11,138)],4.2,1.0,'web','spine_02'))
        for y in [-8,0,8]:
            p.append(bevelbox('Magazine_pouch',(15.3,y,119),(2.8,3.2,6),'uniform','spine_01',.85))
            p.append(bevelbox('Pouch_flap',(18.3,y,122),(.45,3.3,2.1),'web','spine_01',.5))
            p.append(bevelbox('Pouch_snap',(18.85,y,120.8),(.16,.55,.55),'plate','spine_01',.3))
        p.append(bevelbox('Response_chest_patch',(15.75,0,138),(.2,6,1.5),'accent','spine_02',.25))
        for z in [128,131.1]:
            p.append(strap('MOLLE_webbing',[(16,-10,z),(16.1,0,z),(16,10,z)],1.0,.35,'uniform','spine_02'))
        p.append(bevelbox('Radio',(-4,19.2,132),(3,2.7,6),'web','spine_02',.8))
        p.append(tube('Radio_antenna',(-4,19.2,137),(-4,19.2,154),[(0,.4,.4),(1,.22,.22)],'rubber','spine_02',sides=10))
        p.append(bevelbox('Medical_utility_pack',(-14,0,112),(4,9.5,6),'uniform','spine_01',1.4))
    p.append(loft('Utility_belt',[(100,-.2,0,11.3,16.3),(104,-.2,0,11.3,16.3)],'web',weight('pelvis'),40))
    p.append(bevelbox('Belt_buckle',(11.4,0,102),(.7,2.3,1.8),'plate','pelvis',.3))
    # Individually shaped thighs, knee folds, shins and curved boot surfaces.
    for side,sgn in [('l',-1),('r',1)]:
        tb='thigh_'+side; cb='calf_'+side; fb='foot_'+side
        a,b,_=bones[tb]; _,c,_=bones[cb]
        p.append(tube('Trouser_thigh_'+side,a,b,[(0,8.6,8.3),(.13,10,9.3),(.32,9.6,8.7),(.58,8,7.3),(.8,6.6,6.5),(.95,6.2,6.1),(1,6.4,6.15)],cloth,tb,cb,sides=28))
        p.append(tube('Trouser_lower_'+side,b,c,[(0,6.4,6.1),(.12,6.7,6.4),(.26,6.6,6.3),(.47,5.9,5.6),(.73,4.7,4.5),(.91,4.6,4.4),(1,4.8,4.6)],cloth,cb,fb,sides=28))
        p.append(bevelbox('Cargo_pocket_'+side,(-1,sgn*17.5,80),(5.6,1.0,8),'orange' if infected else 'uniform',tb,1))
        p.append(bevelbox('Cargo_flap_'+side,(-1,sgn*18.6,85),(5.6,.45,2),'orange' if infected else 'web',tb,.55))
        if not infected:
            p.append(bevelbox('Patella_guard_'+side,(7.5,sgn*9,54),(2.3,5.9,6.4),'plate',cb,2))
            p.append(bevelbox('Knee_guard_insert_'+side,(10,sgn*9,54),(.5,3.3,3.6),'web',cb,1.3))
        else:
            p.append(loft('Trouser_reflective_band_'+side,[(26,0,sgn*9,5.25,5.0),(29,0,sgn*9,5.5,5.3)],'reflective',weight(cb),28))
        p.append(loft('Boot_shaft_'+side,[(5,1.6,sgn*9,7.4,5.5),(10,-.1,sgn*9,5.8,5.1),(16,-.4,sgn*9,5.3,4.8),(24,-.5,sgn*9,5.45,4.9)],'rubber',weight(fb),28))
        # Toe volume deliberately broader and flatter than ankle, protective toe and sole.
        p.append(loft('Boot_foot_'+side,[(1.1,5.5,sgn*9,12.5,5.8),(2.9,5.5,sgn*9,12.5,5.8),(5.0,5.8,sgn*9,11.9,5.6),(7.8,5.2,sgn*9,10.4,5.3),(10.2,2.2,sgn*9,6.0,4.8)],'web',weight(fb),32))
        p.append(loft('Boot_sole_'+side,[(.6,5.7,sgn*9,12.9,6.0),(2.3,5.7,sgn*9,12.9,6.0),(3.3,5.7,sgn*9,12.7,5.9)],'rubber',weight(fb),32))
        for z in [12,15,18,21]:
            p.append(strap('Boot_lacing',[(5.3,sgn*9-2,z),(5.5,sgn*9+2,z+1)],.45,.35,'uniform' if not infected else 'web',fb))
    return p

def join_skinned(parts,name,rig):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts: o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join()
    o=bpy.context.object; o.name=name
    # Joined primitive transforms become vertex coordinates; rig and object stay unit scale.
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    # Original vertex-painted wear; no image library or third-party textures.
    paint=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
    cloth_names={'M_UniformSlate','M_OrangeFabric','M_Webbing'}
    for polygon in o.data.polygons:
        material=o.data.materials[polygon.material_index]
        cloth=material.name in cloth_names
        for li in polygon.loop_indices:
            v=o.data.vertices[o.data.loops[li].vertex_index].co
            x,y,z=v
            fine=(math.sin(x*2.23+y*1.77+z*1.35)+math.sin(x*1.1-y*3.17+z*.87))/2
            broad=(math.sin(x*.16+z*.13)*math.sin(y*.22-z*.11))
            level=1.0
            if cloth:
                level=.87+.075*fine+.09*broad
                # Lower-leg grime and creases at knee and elbow height.
                if z<38: level*=.77+.23*max(0,z/38)
                if 47<z<59: level*=.90+.10*math.sin(z*1.9+y*.2)**2
                if material.name=='M_OrangeFabric':
                    dirt=max(0,math.sin(x*.31+y*.18+z*.11)*math.sin(z*.27-y*.44)-.35)
                    level*=1-dirt*.65
            elif material.name in {'M_Skin','M_InfectedSkin'}:
                level=.9+.07*broad+.03*fine
            blood=0
            if material.name=='M_OrangeFabric':
                for cx,cy,cz,ry,rz in [(11,-8,129,5,9),(10,-10,117,4,5),(8,13,139,3,4),(10,-3,105,4,3),(7,9,55,4,7)]:
                    distance=((y-cy)/ry)**2+((z-cz)/rz)**2
                    irregular=.75+.19*math.sin(y*1.2+z*.9)+.13*math.sin(z*2.1)
                    if x>cx-3 and distance<irregular:
                        blood=max(blood,min(1,(irregular-distance)*3))
            paint.data[li].color=(level*(1-.18*blood),level*(1-.80*blood),level*(1-.65*blood),1)
    # Blender preview uses exactly the vertex-color multiplier available to Unreal.
    for material in o.data.materials:
        if not material.node_tree.nodes.get('Original_vertex_paint'):
            nodes=material.node_tree.nodes; links=material.node_tree.links
            color=nodes.new('ShaderNodeVertexColor'); color.layer_name='Color'; color.name='Original_vertex_paint'
            multiply=nodes.new('ShaderNodeMixRGB'); multiply.blend_type='MULTIPLY'; multiply.inputs[0].default_value=1
            multiply.inputs[1].default_value=material.diffuse_color
            links.new(color.outputs['Color'],multiply.inputs[2]); links.new(multiply.outputs[0],nodes.get('Principled BSDF').inputs['Base Color'])
    mod=o.modifiers.new('Reusable weighted skeleton','ARMATURE'); mod.object=rig
    o.parent=rig; o.matrix_parent_inverse=Matrix.Identity(4)
    return o

def fbx(path,objects,animations=False):
    if HEAD_ONLY and path.stem not in {'SK_Infected_Head','SM_Infected_Head'}: return
    if BODY_ONLY and path.stem not in {'SK_Response','SK_Infected','SK_Infected_ArmL','SM_Infected_ArmL'}: return
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects: o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'},
        axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS',use_space_transform=True,bake_space_transform=False,
        add_leaf_bones=False,primary_bone_axis='Y',secondary_bone_axis='X',
        use_armature_deform_only=False,mesh_smooth_type='FACE',use_mesh_modifiers=True,
        bake_anim=animations,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_simplify_factor=0,
        path_mode='AUTO')

def zero_pose(rig):
    for b in rig.pose.bones:
        b.location=(0,0,0); b.rotation_mode='QUATERNION'; b.rotation_quaternion=(1,0,0,0); b.scale=(1,1,1)

def rot(rig,name,axis,angle):
    """Armature-world rotation around a bone's current head."""
    bpy.context.view_layer.update(); b=rig.pose.bones[name]
    m=b.matrix.copy(); h=m.translation.copy()
    b.matrix=Matrix.Translation(h) @ Quaternion(Vector(axis),angle).to_matrix().to_4x4() @ Matrix.Translation(-h) @ m
    bpy.context.view_layer.update()

def move(rig,name,delta):
    bpy.context.view_layer.update(); b=rig.pose.bones[name]
    m=b.matrix.copy(); m.translation+=Vector(delta); b.matrix=m
    bpy.context.view_layer.update()

def segment(rig,name,head,tail):
    b=rig.pose.bones[name]; rest=rig.data.bones[name]
    q=(rest.tail_local-rest.head_local).normalized().rotation_difference((Vector(tail)-Vector(head)).normalized())
    basis=q.to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4(); basis.translation=Vector(head)
    b.matrix=basis; bpy.context.view_layer.update()

def solve_limb(rig,upper,lower,tip,goal,pole):
    bpy.context.view_layer.update()
    a=rig.pose.bones[upper].head.copy(); goal=Vector(goal); pole=Vector(pole)
    l1=rig.data.bones[upper].length; l2=rig.data.bones[lower].length
    delta=goal-a; distance=min(delta.length,l1+l2-.01); d=delta.normalized()
    along=(l1*l1-l2*l2+distance*distance)/(2*distance)
    height=math.sqrt(max(0,l1*l1-along*along))
    bend=(pole-a)-d*(pole-a).dot(d)
    if bend.length<.01: bend=Vector((1,0,0))
    b=a+d*along+bend.normalized()*height
    segment(rig,upper,a,b); segment(rig,lower,b,goal)
    return b

def anim(rig,kind,name,seconds):
    scene=bpy.context.scene; frames=round(seconds*FPS); scene.frame_start=1; scene.frame_end=frames+1
    act=bpy.data.actions.new('A_'+kind+'_'+name); act.use_fake_user=True
    rig.animation_data_create(); rig.animation_data.action=act
    infected=kind=='Infected'; walk=name in ['Walk','Run','Back','StrafeL','StrafeR']
    for frame in range(frames+1):
        scene.frame_set(frame+1); zero_pose(rig)
        t=frame/frames; phase=t*math.tau
        # Slow breath from pelvis to shoulders, restrained enough for top-down aim.
        rig.pose.bones['spine_02'].scale.y=1+.006*math.sin(phase)
        if infected:
            rot(rig,'spine_01',(0,1,0),math.radians(12))
            rot(rig,'spine_02',(0,1,0),math.radians(5))
            rot(rig,'neck',(0,1,0),math.radians(-11))
            rot(rig,'head',(1,0,0),math.radians(-5))
        if walk:
            running=name=='Run'; speed=(195 if running else 100) if infected else (370 if running else 180)
            direction=Vector((1,0,0))
            if name=='Back': direction=Vector((-1,0,0))
            elif name=='StrafeL': direction=Vector((0,-1,0))
            elif name=='StrafeR': direction=Vector((0,1,0))
            stance=.58 if not running else .43
            excursion=speed*seconds*stance
            # Pelvis vertical trajectory + small counter rotation transfer weight.
            move(rig,'pelvis',(0,0,(-7.0 if not running else -10.0)+(1.0 if not running else 2.0)*math.cos(phase*2)))
            rot(rig,'pelvis',(0,0,1),math.radians((2.8 if not running else 4.5)*math.sin(phase)))
            rot(rig,'spine_01',(0,0,1),math.radians((-2.0 if not running else -3.3)*math.sin(phase)))
            foot_targets=[]
            for side,offset,sgn in [('l',0,-1),('r',.5,1)]:
                u=(t+offset)%1
                if u<stance:
                    travel=excursion/2-excursion*u/stance; lift=0
                    pitch=math.radians(8)*max(0,(u/stance-.8)/.2)
                else:
                    v=(u-stance)/(1-stance)
                    smooth=v*v*(3-2*v)
                    travel=-excursion/2+excursion*smooth
                    lift=(13 if not running else 24)*math.sin(math.pi*v)
                    pitch=-math.radians(10)*math.sin(math.pi*v)
                # Shamble is modestly asymmetric rather than random upper-body wobble.
                if infected and side=='l': lift*=.6
                target=Vector((0,sgn*9,10))+direction*travel+Vector((0,0,lift))
                foot_targets.append((side,sgn,target,pitch))
            # Keep both leg chains physically reachable before solving either chain.
            # The support foot remains planted; pelvis absorbs any remaining reach limit.
            bpy.context.view_layer.update(); drop=0.0
            for side,sgn,target,pitch in foot_targets:
                hip=rig.pose.bones['thigh_'+side].head
                length=rig.data.bones['thigh_'+side].length+rig.data.bones['calf_'+side].length-.4
                horizontal=(target.x-hip.x)**2+(target.y-hip.y)**2
                max_hip=target.z+math.sqrt(max(0,length*length-horizontal))
                drop=min(drop,max_hip-hip.z)
            if drop<0: move(rig,'pelvis',(0,0,drop))
            for side,sgn,target,pitch in foot_targets:
                solve_limb(rig,'thigh_'+side,'calf_'+side,'foot_'+side,target,Vector((80,sgn*9,53)))
                foot=rig.pose.bones['foot_'+side]
                rest=rig.data.bones['foot_'+side]
                m=Quaternion(Vector((0,1,0)),pitch).to_matrix().to_4x4() @ rest.matrix_local.to_3x3().to_4x4(); m.translation=target
                foot.matrix=m
            if infected:
                for side,sgn in [('l',-1),('r',1)]:
                    rot(rig,'upperarm_'+side,(0,1,0),math.radians((10 if running else 7)*math.sin(phase+(math.pi if sgn>0 else 0))-8))
                    rot(rig,'lowerarm_'+side,(0,1,0),math.radians(-12 if running else -7))
            else:
                # Rifle-ready torso absorbs locomotion and maintains both grips.
                rot(rig,'spine_02',(0,1,0),math.radians(2.5 if running else 1))
        elif name=='Idle':
            if infected:
                rot(rig,'spine_02',(0,0,1),math.radians(1.4*math.sin(phase)))
                rot(rig,'upperarm_r',(0,1,0),math.radians(-4+2*math.sin(phase)))
                rot(rig,'lowerarm_r',(0,1,0),math.radians(-6))
        elif name=='Fire':
            kick=max(0,math.sin(math.pi*min(t/.42,1)))*(1-t)
            rot(rig,'spine_02',(0,1,0),math.radians(-3.4*kick))
            rot(rig,'upperarm_r',(0,1,0),math.radians(-2*kick))
        elif name=='Reload':
            # Support hand travels to the belt, inserts magazine, works the receiver.
            # Right hand and rifle remain a stable unit attached to the shoulder.
            envelope=math.sin(math.pi*t)**1.4
            rot(rig,'spine_02',(0,0,1),math.radians(5*envelope))
            rot(rig,'upperarm_r',(0,1,0),math.radians(-10*envelope))
            points=[(0,(42,5,137)),(.16,(27,-12,124)),(.35,(14,-8,106)),(.52,(24,5,120)),(.71,(24,5,125)),(.83,(36,5,137)),(1,(42,5,137))]
            for i in range(len(points)-1):
                if points[i][0]<=t<=points[i+1][0]:
                    u=(t-points[i][0])/(points[i+1][0]-points[i][0]); u=u*u*(3-2*u)
                    goal=Vector(points[i][1]).lerp(Vector(points[i+1][1]),u); break
            solve_limb(rig,'upperarm_l','lowerarm_l','hand_l',goal,(10,-45,115))
        elif name in ['Attack','AttackOneArm']:
            # Telegraph .0-.3; strike .3-.48; recovery .48-1.0. Right arm remains usable.
            if t<.30: u=t/.30; ang=15*u; extension=0
            elif t<.48: u=(t-.30)/.18; ang=15-86*(u*u*(3-2*u)); extension=u
            else: u=(t-.48)/.52; ang=-71*(1-u*u*(3-2*u)); extension=1-u
            rot(rig,'spine_01',(0,0,1),math.radians(-7*math.sin(math.pi*t)))
            rot(rig,'spine_02',(0,1,0),math.radians(6*extension))
            rot(rig,'upperarm_r',(0,1,0),math.radians(ang))
            rot(rig,'lowerarm_r',(0,1,0),math.radians(-18*(1-extension)))
            if name=='Attack': rot(rig,'upperarm_l',(0,1,0),math.radians(-20*math.sin(math.pi*t)))
        elif name=='Hit':
            e=math.sin(math.pi*t)**1.1
            rot(rig,'spine_01',(0,1,0),math.radians(-12*e))
            rot(rig,'spine_02',(0,0,1),math.radians(-6*e))
            rot(rig,'head',(0,1,0),math.radians(-10*e))
        elif name=='Death':
            # Full skeletal fall: pelvis tips backward, knees fold and shoulder lands.
            u=min(1,t/.87); eased=u*u*(3-2*u)
            hip_z=13+82*max(0,math.cos(math.radians(103*eased)))
            move(rig,'pelvis',(-29*eased,0,hip_z-95))
            rot(rig,'pelvis',(0,1,0),math.radians(-103*eased))
            rot(rig,'pelvis',(1,0,0),math.radians(-10*eased))
            rot(rig,'thigh_l',(0,1,0),math.radians(30*eased))
            rot(rig,'calf_l',(0,1,0),math.radians(-39*eased))
            rot(rig,'thigh_r',(0,1,0),math.radians(20*eased))
            rot(rig,'calf_r',(0,1,0),math.radians(-20*eased))
            rot(rig,'upperarm_l',(1,0,0),math.radians(-29*eased))
            rot(rig,'upperarm_r',(1,0,0),math.radians(37*eased))
            rot(rig,'lowerarm_r',(0,1,0),math.radians(-25*eased))
            # Author floor contact into the baked clip using evaluated skin, including boots.
            bpy.context.view_layer.update(); deps=bpy.context.evaluated_depsgraph_get()
            floor=min(v.co.z for o in rig.children if o.type=='MESH' for v in o.evaluated_get(deps).data.vertices)
            if floor<.6: move(rig,'pelvis',(0,0,.6-floor))
        bpy.context.view_layer.update()
        for b in rig.pose.bones:
            b.keyframe_insert('location',frame=frame+1,group=b.name)
            b.keyframe_insert('rotation_quaternion',frame=frame+1,group=b.name)
            b.keyframe_insert('scale',frame=frame+1,group=b.name)
    scene.frame_set(1)
    fbx(OUT/(act.name+'.fbx'),[rig],True)
    return act

characters={}; manifest={'units':'centimetres','forward':'+X','up':'+Z','fps':FPS,'characters':{}}
for kind in ['Response','Infected']:
    bpy.ops.object.select_all(action='DESELECT')
    rig,bones=make_rig(kind)
    body=body_parts(kind,bones); head=face_parts(kind)
    arm_l=arm_parts(kind,bones,'l'); arm_r=arm_parts(kind,bones,'r')
    if kind=='Response': objects=[join_skinned(body+head+arm_l+arm_r,'SK_Response',rig)]
    else:
        objects=[join_skinned(body+arm_r,'SK_Infected',rig),join_skinned(head,'SK_Infected_Head',rig),join_skinned(arm_l,'SK_Infected_ArmL',rig)]
    for o in objects: fbx(OUT/(o.name+'.fbx'),[rig,o])
    detaches={}
    if kind=='Infected':
        for o,bone,name in [(objects[1],'head','SM_Infected_Head'),(objects[2],'upperarm_l','SM_Infected_ArmL')]:
            cp=o.copy(); cp.data=o.data.copy(); bpy.context.collection.objects.link(cp)
            cp.name=name; cp.parent=None; cp.modifiers.clear(); cp.vertex_groups.clear()
            inverse=rig.data.bones[bone].matrix_local.inverted()
            for v in cp.data.vertices: v.co=inverse@v.co
            fbx(OUT/(name+'.fbx'),[cp]); cp.hide_render=True; cp.hide_set(True)
            detaches[name]={'bone':bone,'bone_matrix':[list(row) for row in rig.data.bones[bone].matrix_local]}
    definitions={'Idle':3.0,'Walk':.6,'Run':.5,'Back':.6,'StrafeL':.6,'StrafeR':.6,'Fire':.2,'Reload':2.1} if kind=='Response' else {'Idle':3.0,'Walk':1.4,'Run':.8,'Attack':1.0,'AttackOneArm':1.0,'Hit':.4,'Death':1.2}
    actions={}
    for name,duration in definitions.items(): actions[name]=anim(rig,kind,name,duration)
    characters[kind]=(rig,objects,actions)
    manifest['characters'][kind]={'meshes':{o.name:{'vertices':len(o.data.vertices),'triangles':sum(len(p.vertices)-2 for p in o.data.polygons),'materials':[m.name for m in o.data.materials]} for o in objects},'clips':definitions,'bones':{n:{'head':list(h),'tail':list(t),'parent':p} for n,(h,t,p) in bones.items()},'detached':detaches}
    rig.animation_data.action=actions['Idle']; bpy.context.scene.frame_set(1)
    # Save source with this character visible and the other hidden, all animation retained.
    for k,(r,objs,acts) in characters.items():
        for o in [r]+objs: o.hide_render=k!=kind; o.hide_set(k!=kind)
    bpy.ops.wm.save_as_mainfile(filepath=str(SRC/(kind+'.blend')))

# Character workshop scene: actual authored assets, labelled Blender QA only.
for kind,(rig,objects,actions) in characters.items():
    rig.animation_data.action=actions['Idle']; rig.location.y=-65 if kind=='Response' else 65
    rig.hide_set(False); rig.hide_render=False
    for o in objects: o.hide_set(False); o.hide_render=False
scene=bpy.context.scene; scene.frame_set(1)
world=bpy.data.worlds.new('Workshop_world'); world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.11,.13,.15,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.55; scene.world=world
def area(name,pos,power,size):
    d=bpy.data.lights.new(name,'AREA'); d.energy=power; d.shape='DISK'; d.size=size
    o=bpy.data.objects.new(name,d); bpy.context.collection.objects.link(o); o.location=pos
    o.rotation_euler=(Vector((0,0,95))-o.location).to_track_quat('-Z','Y').to_euler()
area('Softbox_key',(220,-160,290),500000,180)
area('Softbox_fill',(80,230,185),220000,160)
area('Rim',(-100,40,240),350000,120)
camd=bpy.data.cameras.new('Workshop_camera'); cam=bpy.data.objects.new('Workshop_camera',camd); bpy.context.collection.objects.link(cam)
cam.location=(350,-310,235); target=Vector((0,0,96)); cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
camd.type='ORTHO'; camd.ortho_scale=295; camd.clip_end=2000; scene.camera=cam
scene.render.engine='CYCLES'; scene.cycles.samples=24
scene.render.resolution_x=1400; scene.render.resolution_y=1100; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.film_transparent=False
scene.view_settings.view_transform='AgX'
scene.render.filepath=str(SRC/'CharacterWorkshop.png')
bpy.ops.wm.save_as_mainfile(filepath=str(SRC/'CharacterWorkshop.blend'))
bpy.ops.render.render(write_still=True)
(SRC/'manifest.json').write_text(json.dumps(manifest,indent=2))
(SRC/'materials.json').write_text(json.dumps({m.name:{'color':list(m.diffuse_color),'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value,'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value} for m in M.values()},indent=2))
print('PROJECT_ONE_CHARACTERS_COMPLETE',json.dumps({k:v['meshes'] for k,v in manifest['characters'].items()}))
