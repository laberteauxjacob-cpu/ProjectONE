"""Original C07 facility-worker variants on the unchanged infected bind rig.

Blender 5.1: --background --factory-startup --python this_file -- --variant maintenance --render
Writes only Candidate07 character/export paths. No player/old asset is changed.
No third-party inputs. Deliberate geometry and paint are editable in this source.
"""
from pathlib import Path
import argparse, hashlib, json, math, random, sys
import bpy
from mathutils import Matrix, Vector
from io_scene_fbx import parse_fbx

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'ArtSource/Characters/Candidate07'
EXPORT = ROOT/'ArtSource/Exports/Candidate07'
parser = argparse.ArgumentParser()
parser.add_argument('--variant', choices=['maintenance','laboratory','facility_staff'], default='maintenance')
parser.add_argument('--render', action='store_true')
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
CONFIG = json.loads((SOURCE/(args.variant+'_design.json')).read_text())
VARIANT={'maintenance':'Maintenance','laboratory':'Laboratory','facility_staff':'FacilityStaff'}[args.variant]
LAB=args.variant=='laboratory';STAFF=args.variant=='facility_staff'
EXPORT.mkdir(parents=True, exist_ok=True)
GUARD_PATHS = ['ArtSource/Characters/Response.blend', 'ArtSource/Characters/C05/ResponseMotion.blend',
               'ArtSource/Characters/Infected.blend', CONFIG['rig_source']]
if args.variant!='maintenance':
    preserved=json.loads((SOURCE/'maintenance_inventory.json').read_text())
    GUARD_PATHS += [preserved['editable_source'],'ArtSource/Characters/Candidate07/maintenance_inventory.json','ArtSource/Characters/Candidate07/maintenance_validation.json']
    GUARD_PATHS += [d['source'] for d in preserved['meshes'].values()]+[d['source'] for d in preserved['textures'].values()]
GUARDS = {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in GUARD_PATHS}
bpy.ops.wm.open_mainfile(filepath=str(ROOT/CONFIG['rig_source']))
rig = bpy.data.objects[CONFIG['rig_object']]
for ob in list(bpy.data.objects):
    if ob != rig: bpy.data.objects.remove(ob, do_unlink=True)
rig.animation_data_clear()
for b in rig.pose.bones: b.matrix_basis = Matrix.Identity(4)
REST = {b.name: b.matrix_local.copy() for b in rig.data.bones}
assert len(REST) == 21 and all(abs(s-1)<1e-8 for s in rig.scale)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'; scene.unit_settings.scale_length = .01
scene.render.fps = 30; scene.frame_set(1)
PREFIX = 'SK_Infected_C07_'+VARIANT+'_'

def clamp(x, lo=0., hi=1.): return max(lo, min(hi, x))
def smooth(x):
    x=clamp(x); return x*x*(3-2*x)
def mix(a,b,t): return tuple(x*(1-t)+y*t for x,y in zip(a,b))
def gaussian(x, center, width): return math.exp(-((x-center)/width)**2)
def blend_weights(a,b,t):
    t=clamp(t); return {k:v for k,v in {a:1-t,b:t}.items() if v>1e-8}
def rigid(bone): return lambda p: {bone:1.}
def torso_weights(p):
    z=p.z
    if z < 104: return {'pelvis':1.}
    if z < 117: return blend_weights('pelvis','spine_01',smooth((z-104)/13))
    return blend_weights('spine_01','spine_02',smooth((z-119)/16))

PALETTE = {
 'skin': ('M_C07_InfectedSkin',(.40,.325,.255),.83,0),
 'cloth': ('M_C07_WorkCloth',(.10,.155,.18),.92,0),
 'hair': ('M_C07_Hair',(.027,.023,.018),.92,0),
 'boot': ('M_C07_Boot',(.033,.030,.026),.86,0),
 'flesh': ('M_C07_CutTissue',(.22,.033,.034),.48,0),
 'bone': ('M_C07_BoneTeeth',(.48,.40,.245),.78,0),
 'eye': ('M_C07_CloudedEye',(.39,.40,.31),.40,0),
 'metal': ('M_C07_WorkMetal',(.14,.14,.12),.65,.35),
}
if LAB:
    PALETTE['skin']=('M_C07_InfectedSkin',(.35,.31,.28),.87,0)
    PALETTE['cloth']=('M_C07_WorkCloth',(.49,.475,.39),.95,0)
    PALETTE['hair']=('M_C07_Hair',(.12,.11,.087),.96,0)
elif STAFF:
    PALETTE['skin']=('M_C07_InfectedSkin',(.34,.25,.19),.82,0)
    PALETTE['cloth']=('M_C07_WorkCloth',(.27,.25,.205),.94,0)
    PALETTE['hair']=('M_C07_Hair',(.024,.018,.016),.94,0)
if args.variant!='maintenance':
    PALETTE={key:(name.replace('M_C07_','M_C07_'+VARIANT+'_'),color,rough,metal) for key,(name,color,rough,metal) in PALETTE.items()}
    PALETTE['trouser']=('M_C07_'+VARIANT+'_Trousers',(.10,.115,.12) if LAB else (.15,.115,.077),.94,0)
MATS={}
DETAIL_TEXTURES={}
for key,size,frequency,strength in [('skin',256,19,.065),('cloth',256,24,.20)]:
    if args.variant!='maintenance':
        name='T_C07_'+('SkinPores' if key=='skin' else 'WorkWeave')+'_N'
        texture=bpy.data.images.load(str(SOURCE/(name+'.png')),check_existing=True)
        texture.name=name;texture.colorspace_settings.name='Non-Color';DETAIL_TEXTURES[key]=texture
        continue
    # Original tileable tangent normals: periodic pores or woven warp/weft.
    def height(px,py):
        x=math.tau*px/size;y=math.tau*py/size
        if key=='cloth':return .5*math.sin(x*frequency)*(.7+.3*math.cos(y*frequency))+.38*math.sin(y*frequency)*(.7+.3*math.cos(x*frequency))
        gx=px/size*16;gy=py/size*16;ix=math.floor(gx);iy=math.floor(gy)
        def rand(a,b,salt):
            value=math.sin((a%16)*127.1+(b%16)*311.7+salt)*43758.5453
            return value-math.floor(value)
        pore=0.
        for oy in (-1,0,1):
            for ox in (-1,0,1):
                cx=ix+ox+.15+.70*rand(ix+ox,iy+oy,1.3);cy=iy+oy+.15+.70*rand(ix+ox,iy+oy,8.7)
                pore+=math.exp(-((gx-cx)**2+(gy-cy)**2)/.032)
        return -.75*pore+.045*math.sin(47*x+31*y)*math.sin(59*x-43*y)
    pixels=[]
    for y in range(size):
        for x in range(size):
            dx=(height(x+1,y)-height(x-1,y))*strength;dy=(height(x,y+1)-height(x,y-1))*strength
            n=Vector((-dx,-dy,1)).normalized();pixels.extend((n.x*.5+.5,n.y*.5+.5,n.z*.5+.5,1.))
    name='T_C07_'+('SkinPores' if key=='skin' else 'WorkWeave')+'_N'
    texture=bpy.data.images.new(name,width=size,height=size,alpha=False)
    texture.colorspace_settings.name='Non-Color';texture.pixels.foreach_set(pixels)
    texture.filepath_raw=str(SOURCE/(name+'.png'));texture.file_format='PNG';texture.save()
    DETAIL_TEXTURES[key]=texture
for key,(name,color,rough,metal) in PALETTE.items():
    m=bpy.data.materials.new(name); m.use_nodes=True; m.diffuse_color=(*color,1)
    nodes=m.node_tree.nodes; links=m.node_tree.links; principled=nodes.get('Principled BSDF')
    attr=nodes.new('ShaderNodeVertexColor'); attr.layer_name='Color'; attr.name='C07_original_colour_and_roughness'
    links.new(attr.outputs['Color'],principled.inputs['Base Color'])
    links.new(attr.outputs['Alpha'],principled.inputs['Roughness'])
    principled.inputs['Metallic'].default_value=metal
    detail_key='cloth' if key=='trouser' else key
    if detail_key in DETAIL_TEXTURES:
        image=nodes.new('ShaderNodeTexImage');image.image=DETAIL_TEXTURES[detail_key];image.extension='REPEAT'
        normal=nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=1.
        links.new(image.outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],principled.inputs['Normal'])
    MATS[key]=m

def paint(key,p,accent=None):
    base=accent or PALETTE[key][1]; rough=PALETTE[key][2]
    if args.variant!='maintenance' and key in ('cloth','hair','trouser'):
        shade=clamp(sum(accent)/max(.001,sum((.10,.155,.18) if key=='cloth' else (.064,.048,.029))),.60,1.24) if accent else 1.
        base=tuple(c*shade for c in PALETTE[key][1])
    x,y,z=p
    fine=math.sin(x*3.19+y*4.7+z*2.43)*math.sin(y*2.83-z*4.19)
    patch=math.sin(x*.37+z*.19)*math.sin(y*.23-z*.29)
    color=tuple(c*(.92+.055*fine+.055*patch) for c in base)
    if key=='skin':
        # Spatially localized decay: grey-purple temples, sallow high planes,
        # irritated scars and dried crust do not tint all flesh one colour.
        decay=clamp(.90*gaussian(z,169,4)*gaussian(abs(y),5.0,2.4)*clamp(x/6)+
                    .42*gaussian(z,103,12)*gaussian(y,26,5)+
                    .28*gaussian(z,176,2)*gaussian(y,-3,3))
        color=mix(color,(.11,.054,.045),decay)
        eye_bruise=gaussian(z,170.65,1.1)*gaussian(abs(y),3.1,1.3)*clamp((x-2)/3)
        color=mix(color,(.19,.075,.115),eye_bruise*.95)
        temple_mask=gaussian(z,172.5,2.0)*gaussian(abs(y),5.6,1.8)*clamp((x+1)/5)
        color=mix(color,(.14,.11,.145),temple_mask*.66)
        jaw_mask=gaussian(z,162.5,2.0)*gaussian(abs(y),3.6,2.3)*clamp((x+1)/5)
        color=mix(color,(.17,.185,.17),jaw_mask*.60)
        nostril=gaussian(abs(y),.80,.25)*gaussian(z,167.30,.20)*clamp((x-5.8)/.8)
        color=mix(color,(.035,.017,.012),nostril*.94)
        lip_stain=gaussian(y,.35+.18*(164-z),1.6)*gaussian(z,163.9,1.0)*clamp((x-4)/2)
        color=mix(color,(.17,.065,.04),lip_stain*.64)
        crust=gaussian(y,4.3+.22*(z-165),.7)*gaussian(z,165.5,3.2)*clamp((x-3)/2)
        crust=clamp(crust*(1+.6*fine)-.16)
        color=mix(color,(.10,.031,.025),crust*.95)
        color=mix(color,(.22,.215,.16),clamp(patch-.2)*.36)
        fleck=clamp(math.sin(x*8.1+y*5.3+z*6.7)*math.sin(z*4.9-y*7.1)-.42)*1.7
        color=mix(color,(.075,.035,.022),fleck*decay*.75)
        veins=clamp((math.sin(z*1.7+y*.9+x*.8)-.84)*3)*gaussian(z,105,15)
        color=mix(color,(.205,.19,.19),veins*.45)
        scar=gaussian(y,4.8+.18*(z-168),.34)*gaussian(z,168,3.8)*clamp((x-4)/2)
        color=mix(color,(.48,.355,.28),scar*.75)
        scalp=smooth((z-170.8)/1.1)*smooth((177.5-z)/1.4)*smooth((.8-x)/2.2)
        scalp*=clamp(.55+.40*math.sin(y*1.7+z*2.3)+.18*fine)
        color=mix(color,(.095,.075,.047),scalp*.66)
    if key in ('cloth','trouser'):
        dirt=.26*clamp((45-z)/35)+.20*clamp(-patch)
        color=mix(color,(.075,.065,.043),dirt)
        stains=0.
        patches=([(-10,139,6,11),(-5,124,3,18),(24,120,5,16),(8,88,6,10)] if LAB else [(4,139,4,11),(2,122,3,18),(-8,105,7,6),(-25,129,5,7)] if STAFF else [(-7,132,6,10),(9,114,3.2,7),(10,67,4,12),(-27,130,4,10)])
        for cy,cz,sy,sz in patches:
            stains=max(stains,gaussian(y,cy,sy)*gaussian(z,cz,sz)*clamp((x+2)/8))
        edge=clamp(stains*(1+.45*patch+.65*fine)*2.8-.90)
        color=mix(color,(.055,.018,.009),edge*.98)
        rough-=edge*.12
    if key=='boot':
        mud=(.62*gaussian(z,3.0,3.0)+.48*gaussian(z,7,3)*clamp((x-4)/9))*(.73+.27*patch)
        color=mix(color,(.15,.115,.065),clamp(mud))
        scuff=clamp(fine-.20)*gaussian(z,10,6)*clamp((x-2)/8)
        color=mix(color,(.22,.18,.12),scuff*.40)
    rough=clamp(rough+.035*fine,.12,.98)
    return (*color,rough)

class Geometry:
    def __init__(self,name):
        self.name=name; self.v=[]; self.w=[]; self.f=[]; self.mat=[]; self.colors=[]; self.seams={}; self.features={}
    def vertex(self,p,weights):
        self.v.append(Vector(p)); self.w.append({k:v for k,v in weights.items() if v>1e-8}); return len(self.v)-1
    def face(self,indices,key,accent=None):
        self.f.append(tuple(indices)); self.mat.append(key); self.colors.append(accent)
    def ring(self,coords,wf): return [self.vertex(p,wf(Vector(p))) for p in coords]
    def bridge(self,a,b,key,accent=None):
        assert len(a)==len(b)
        for i in range(len(a)): j=(i+1)%len(a); self.face((a[i],a[j],b[j],b[i]),key,accent)
    def cap(self,ring,key,reverse=False,accent=None): self.face(list(reversed(ring)) if reverse else ring,key,accent)
    def feature(self,name,**data): self.features[name]=data
    def object(self):
        mesh=bpy.data.meshes.new(self.name+'_Geometry'); mesh.from_pydata(self.v,[],self.f); mesh.update()
        ob=bpy.data.objects.new(self.name,mesh); scene.collection.objects.link(ob)
        keys=list(dict.fromkeys(self.mat))
        for k in keys: mesh.materials.append(MATS[k])
        for bone in REST:
            if any(bone in w for w in self.w): ob.vertex_groups.new(name=bone)
        for i,w in enumerate(self.w):
            assert abs(sum(w.values())-1)<1e-6
            for b,weight in w.items(): ob.vertex_groups[b].add([i],weight,'REPLACE')
        colors=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='CORNER')
        uv=mesh.uv_layers.new(name='C07_DetailUV')
        for polygon,key,accent in zip(mesh.polygons,self.mat,self.colors):
            polygon.material_index=keys.index(key); polygon.use_smooth=len(polygon.vertices)==4
            axis=max(range(3),key=lambda i:abs(polygon.normal[i]))
            for li in polygon.loop_indices:
                p=mesh.vertices[mesh.loops[li].vertex_index].co
                colors.data[li].color=paint(key,p,accent)
                axes=[i for i in range(3) if i!=axis]
                tile_cm=1.6 if key=='skin' else 1.8
                uv.data[li].uv=(p[axes[0]]/tile_cm,p[axes[1]]/tile_cm)
        marker=mesh.attributes.new(name='C07Seam',type='INT',domain='POINT')
        for sid,indices in self.seams.items():
            for i in indices: marker.data[i].value=sid
        ob.parent=rig; ob.matrix_parent_inverse=Matrix.Identity(4)
        mod=ob.modifiers.new('Unchanged infected bind','ARMATURE'); mod.object=rig
        ob['C07_original_geometry']=True; ob['feature_inventory']=json.dumps(self.features)
        return ob

G={name:Geometry(PREFIX+name) for name in ('Core','Head','ArmLeft','ArmRight','LegLeft')}

def frame_axis(direction):
    n=Vector(direction).normalized(); u=Vector((1,0,0));u=(u-u.dot(n)*n).normalized();return n,u,n.cross(u).normalized()
def cross_ring(center,direction,rx,ry,sides=32,phase=0):
    n,u,v=frame_axis(direction); c=Vector(center)
    return [c+u*(math.cos(math.tau*i/sides+phase)*rx)+v*(math.sin(math.tau*i/sides+phase)*ry) for i in range(sides)]
def sample_profile(t,keys):
    for a,b in zip(keys,keys[1:]):
        if t<=b[0]:
            q=clamp((t-a[0])/(b[0]-a[0])); return tuple(x*(1-q)+y*q for x,y in zip(a[1:],b[1:]))
    return keys[-1][1:]
def skin_tube(g,points,key,wf,sides=24,caps=True,accent=None):
    rings=[]
    for j,(center,r1,r2) in enumerate(points):
        direction=Vector(points[min(j+1,len(points)-1)][0])-Vector(points[max(0,j-1)][0])
        rings.append(g.ring(cross_ring(center,direction,r1,r2,sides),wf))
    for a,b in zip(rings,rings[1:]):g.bridge(a,b,key,accent)
    if caps:g.cap(rings[0],key,True,accent);g.cap(rings[-1],key,False,accent)
    return rings
def ellipsoid(g,name,center,scale,key,bone,segments=20,rings=10,accent=None):
    c=Vector(center); row=[]
    for j in range(rings+1):
        a=.001+(math.pi-.002)*j/rings
        pts=[c+Vector((scale[0]*math.sin(a)*math.cos(math.tau*i/segments),scale[1]*math.sin(a)*math.sin(math.tau*i/segments),scale[2]*math.cos(a))) for i in range(segments)]
        row.append(g.ring(pts,rigid(bone)))
    for a,b in zip(row,row[1:]):g.bridge(a,b,key,accent)
    g.cap(row[0],key,True,accent);g.cap(row[-1],key,False,accent)
    g.feature(name,kind='modeled_volume')
def ribbon(g,name,points,width,thickness,key,wf,accent=None):
    pts=[Vector(p) for p in points]; rows=[]
    for j,p in enumerate(pts):
        d=(pts[min(j+1,len(pts)-1)]-pts[max(0,j-1)]).normalized()
        cross=d.cross(Vector((1,0,0))).normalized()
        if cross.length<.1:cross=Vector((0,1,0))
        rows.append(g.ring([p+cross*width*.5+Vector((thickness*.5,0,0)),p-cross*width*.5+Vector((thickness*.5,0,0)),p-cross*width*.5-Vector((thickness*.5,0,0)),p+cross*width*.5-Vector((thickness*.5,0,0))],wf))
    for a,b in zip(rows,rows[1:]):g.bridge(a,b,key,accent)
    g.cap(rows[0],key,True,accent);g.cap(rows[-1],key,False,accent)
    g.feature(name,kind='thick_sewn_geometry',thickness_cm=thickness)

def cut_cap(g,ring,center,normal,bone,sid,distal):
    """Paired exterior rim with recessed tissue, fascia and inset bone."""
    g.seams.setdefault(sid,[]).extend(ring)
    c=Vector(center);out=-Vector(normal).normalized() if distal else Vector(normal).normalized()
    previous=ring
    for radius,depth,key in [(.92,.12,'flesh'),(.60,.29,'flesh'),(.18,.43,'flesh')]:
        points=[c+(g.v[i]-c)*radius-out*depth for i in ring]
        current=g.ring(points,rigid(bone));g.bridge(previous,current,key);previous=current
    g.cap(previous,'bone',distal)

# Continuous jacket torso with weighted shoulder volumes rooted inside it.
core=G['Core']; sides=64; torso_rows=[]; torso_centers=[]
torso_profile=[(99,-.6,10.4,15.0),(106,-.6,10.1,14.8),(117,-.8,10.2,15.2),(127,-.4,11.6,17.6),(136,-.4,11.2,18.7),(143,-1.1,9.0,19.0),(148,-1,7.0,16.0),(153,-1,4.9,5.8)]
if STAFF:torso_profile=[(z,cx,rx*(.91 if z<137 else 1),ry*(.93 if z<137 else 1)) for z,cx,rx,ry in torso_profile]
if LAB:torso_profile=[(z,cx,rx*(1.035 if z<120 else 1),ry*(1.045 if z<120 else 1)) for z,cx,rx,ry in torso_profile]
for j in range(45):
    z=99+54*j/44;cx,rx,ry=sample_profile(z,torso_profile)
    points=[]
    for i in range(sides):
        a=math.tau*i/sides
        fold=(.34*math.sin(a*8+z*.37)+.22*math.sin(a*13-z*.51))*gaussian(z,110,17)
        fold+=.50*math.sin(z*1.1+a*3)*gaussian(z,136,7)*(abs(math.sin(a))**3)
        fold+=.34*math.sin(a*17+z*.09)*gaussian(z,126,12)*clamp(math.cos(a))
        points.append(Vector((cx+(rx+fold)*math.cos(a),(ry+fold*.8)*math.sin(a),z)))
    torso_rows.append(core.ring(points,torso_weights))
holes={-1:[],1:[]};removed=[]
for j in range(len(torso_rows)-1):
    for i in range(sides):
        k=(i+1)%sides; f=(torso_rows[j][i],torso_rows[j][k],torso_rows[j+1][k],torso_rows[j+1][i])
        c=sum((core.v[v] for v in f),Vector())/4
        if c.x>5 and ((c.y+9.0)/2.3)**2+((c.z-118.0)/3.2)**2<1:removed.append(f)
        else:core.face(f,'cloth')
core.cap(torso_rows[0],'cloth',True);core.cap(torso_rows[-1],'cloth')
edges={}
for f in removed:
    for a,b in zip(f,f[1:]+f[:1]):
        key=tuple(sorted((a,b)));edges[key]=edges.get(key,0)+1
boundary=[e for e,n in edges.items() if n==1]
def ordered_loop(edge_list,geometry=None):
    remaining=set(edge_list); start=min(min(e) for e in remaining); result=[start];now=start
    while remaining:
        e=next(e for e in remaining if now in e);remaining.remove(e);now=e[0] if e[1]==now else e[1]
        if now==start:break
        result.append(now)
    assert not remaining
    if geometry:
        # All authoring holes are on the +X-facing surface. An inward bridge
        # needs positive YZ winding so imported one-sided faces remain visible.
        area=sum(geometry.v[a].y*geometry.v[b].z-geometry.v[a].z*geometry.v[b].y for a,b in zip(result,result[1:]+result[:1]))
        if area<0:result.reverse()
    return result

tear=ordered_loop(boundary,core)
for i in tear:
    p=core.v[i];a=math.atan2((p.z-118)/3.2,(p.y+9)/2.3);r=1+.10*math.sin(a*5)+.05*math.sin(a*9)
    p.y=-9+2.3*r*math.cos(a);p.z=118+3.2*r*math.sin(a)
tc=sum((core.v[i] for i in tear),Vector())/len(tear)
tr=core.ring([tc+(core.v[i]-tc)*.94-Vector((.35,0,0)) for i in tear],torso_weights)
core.bridge(tear,tr,'cloth',(.22,.195,.12))
back=core.ring([tc+(core.v[i]-tc)*.94-Vector((1.1,0,0)) for i in tear],torso_weights)
core.bridge(tr,back,'cloth',(.045,.035,.023));core.cap(back,'cloth',accent=(.035,.032,.020))
for j,i in enumerate(tear):
    if j%2:continue
    p=core.v[i];q=p+Vector((.04,.22*math.sin(j),-.45-.40*abs(math.sin(j*2.3))))
    skin_tube(core,[(p,.04,.035),(q,.015,.015)],'cloth',torso_weights,6,accent=(.25,.23,.155))
core.feature('Torn_chest_workwear',kind='open_cloth_with_thickness_dark_underlayer_and_loose_threads')

def arm_weight(side,p):
    a=Vector(rig.data.bones['upperarm_'+side].head_local);b=Vector(rig.data.bones['lowerarm_'+side].head_local);c=Vector(rig.data.bones['hand_'+side].head_local)
    # Continuous overlap across elbow, unlike two capped tubes with mismatched weights.
    f=(p-b).dot((c-b).normalized())
    if f < -6:return {'upperarm_'+side:1.}
    if f < 7:return blend_weights('upperarm_'+side,'lowerarm_'+side,smooth((f+6)/13))
    wrist=(p-c).dot((c-b).normalized())
    return blend_weights('lowerarm_'+side,'hand_'+side,smooth((wrist+5)/8))

for side,sign,part,sid in [('r',1,'ArmLeft',1),('l',-1,'ArmRight',2)]:
    g=G[part];ub='upperarm_'+side;lb='lowerarm_'+side;hb='hand_'+side
    shoulder=rig.data.bones[ub].head_local.copy();elbow=rig.data.bones[lb].head_local.copy();wrist=rig.data.bones[hb].head_local.copy()
    n=(elbow-shoulder).normalized();cut=shoulder+n*4.8
    count=40
    # A curved deltoid shell starts INSIDE the jacket. Its root is hidden by
    # actual cloth geometry; the sever rim remains a precise shared boundary.
    _,u,v=frame_axis(n)
    first=0.
    rim=[cut+u*(6.55*math.cos(first+math.tau*i/count))+v*(6.15*math.sin(first+math.tau*i/count)) for i in range(count)]
    last_shoulder=None
    for j in range(9):
        t=j/8
        c=Vector((0,sign*11,146)).lerp(cut,t)+Vector((-.15,0,3.7*math.sin(math.pi*t)))
        radiusx=4.2+2.35*smooth(t);radiusy=4.1+2.05*smooth(t)
        coords=[c+u*(radiusx*math.cos(math.tau*i/count))+v*(radiusy*math.sin(math.tau*i/count)) for i in range(count)]
        if j==8:coords=rim
        row=core.ring(coords,lambda p,t=t,ub=ub:blend_weights('spine_02',ub,smooth(t)))
        if last_shoulder is None:core.cap(row,'cloth',True)
        else:core.bridge(last_shoulder,row,'cloth')
        last_shoulder=row
    cut_cap(core,last_shoulder,cut,n,ub,sid,False)
    # One continuous sleeve spans the elbow. Its variable hem has real thickness.
    wf=lambda p,s=side:arm_weight(s,p)
    arm_rim=g.ring(rim,rigid(ub));last=arm_rim
    hem_fraction=(.82 if side=='r' else .58) if LAB else (.05 if side=='r' else .14) if STAFF else (.30 if side=='r' else .82)
    end=elbow.lerp(wrist,hem_fraction);full=(elbow-cut).length+(end-elbow).length
    for row in range(1,35):
        distance=full*row/34
        upper=(elbow-cut).length
        center=cut.lerp(elbow,min(1,distance/upper)) if distance<=upper else elbow.lerp(end,(distance-upper)/(full-upper))
        direction=n if distance<upper-4 else (wrist-elbow).normalized()
        _,ru,rv=frame_axis(direction)
        f=distance/full
        radii=sample_profile(f,[(0,6.55,6.15),(.25,6.5,5.8),(.53,5.2,4.85),(.76,5.0,4.5),(1,4.65,4.25)])
        coords=[]
        for i in range(count):
            a=first+math.tau*i/count
            fold=.43*math.sin(f*math.pi*12+2*math.sin(a))*gaussian(distance,upper-3,10)
            tear=(1.1*math.sin(a*3+.8)+.55*math.sin(a*7))*(smooth((f-.86)/.14)) if side=='r' else .22*math.sin(a*4)*smooth((f-.9)/.1)
            coords.append(center+ru*((radii[0]+fold)*math.cos(a))+rv*((radii[1]+fold)*math.sin(a))+direction*tear)
        ring=g.ring(coords,wf);g.bridge(last,ring,'cloth');last=ring
    inner=g.ring([end+(g.v[i]-end)*.945 for i in last],wf);g.bridge(last,inner,'cloth',(.19,.225,.215));g.cap(inner,'cloth')
    if STAFF:
        skin_tube(g,[(end-(wrist-elbow).normalized()*1.2,4.85,4.5),(end,5.05,4.65),(end+(wrist-elbow).normalized()*.8,4.78,4.4)],'cloth',wf,32,accent=(.13,.15,.14))
        g.feature('Rolled_shirt_cuff',thick_fold_cm=2.0)
    cut_cap(g,arm_rim,cut,n,ub,sid,True)
    # Exposed forearm beneath the torn sleeve, including visible tendons.
    start=elbow.lerp(wrist,max(.05,hem_fraction-.16))
    pts=[]
    for j in range(13):
        f=j/12;c=start.lerp(wrist,f);rx,ry=sample_profile(f,[(0,4.25,3.7),(.3,4.1,3.5),(.67,3.2,2.7),(1,2.7,2.35)])
        pts.append((c,rx,ry))
    skin_tube(g,pts,'skin',wf,24)
    hd=Vector((.20,0,-1)).normalized();palm_end=wrist+hd*7.2
    skin_tube(g,[(wrist,2.3,2.35),(wrist+hd*2,1.85,3.3),(wrist+hd*5,1.65,3.75),(palm_end,1.50,3.55)],'skin',rigid(hb),24)
    for i,length in enumerate((5.6,7.0,6.4,5.0)):
        p=palm_end+Vector((0,(i-1.5)*1.65,0));mid=p+hd*(length*.58)+Vector((-.65,0,.15));tip=mid+hd*(length*.42)+Vector((-1.0,0,.1))
        skin_tube(g,[(p,.88,.83),(mid,.77,.72),(tip,.55,.56)],'skin',rigid(hb),10)
        ellipsoid(g,'Knuckle_'+str(i),p+Vector((1.15,0,.8)),(.45,.7,.75),'skin',hb,12,6)
        ellipsoid(g,'Nail_'+str(i),tip+Vector((.44,0,.5)),(.14,.42,.73),'bone',hb,10,5,accent=(.20,.16,.095))
    thumb=wrist+hd*3+Vector((0,sign*3.0,0))
    skin_tube(g,[(thumb,1.35,1.3),(thumb+Vector((.7,sign*1.6,-2.4)),1.15,1.05),(thumb+Vector((1.2,sign*2.0,-4.8)),.78,.80)],'skin',rigid(hb),16)
    g.feature('Continuous_elbow_sleeve',rows=35,hem='ragged short' if side=='r' else 'worn long',thickness_cm=.25)
    g.feature('Individual_fingers_nails',fingers=5,rig_limit='hand-bone rigid; no new finger joints')

# Pelvic trouser volume overlaps the jacket's hem; continuous knee surfaces.
def horizontal_loft(g,rings,key,wf,sides=40,caps=True,accent=None):
    rows=[g.ring([Vector((cx+rx*math.cos(math.tau*i/sides),cy+ry*math.sin(math.tau*i/sides),z)) for i in range(sides)],wf) for z,cx,cy,rx,ry in rings]
    for a,b in zip(rows,rows[1:]):g.bridge(a,b,key,accent)
    if caps:g.cap(rows[0],key,True,accent);g.cap(rows[-1],key,False,accent)
    return rows
leg_material='cloth' if args.variant=='maintenance' else 'trouser'
horizontal_loft(core,[(89,-.5,0,8.5,14.4),(95,-.7,0,10.2,17.0),(102,-.5,0,10.6,15.1)],leg_material,rigid('pelvis'),40,True,(.10,.125,.13))
for side,sign in [('r',1),('l',-1)]:
    thigh='thigh_'+side;calf='calf_'+side;foot='foot_'+side
    hip=rig.data.bones[thigh].head_local.copy();knee=rig.data.bones[calf].head_local.copy();ankle=rig.data.bones[foot].head_local.copy()
    n=(knee-hip).normalized();cut=hip.lerp(knee,.38)
    def leg_weights(p,thigh=thigh,calf=calf,foot=foot):
        if p.z>62:return {thigh:1.}
        if p.z>44:return blend_weights(thigh,calf,smooth((62-p.z)/18))
        return blend_weights(calf,foot,smooth((21-p.z)/11))
    rows=[]
    # Include the precise cut level as one shared sample, avoiding bisection cracks.
    levels=sorted(set([95-73*j/42 for j in range(43)]+[cut.z]),reverse=True)
    last=None;last_owner=None;cut_row=None
    for z in levels:
        center=hip.lerp(knee,clamp((95-z)/42)) if z>=53 else knee.lerp(ankle,clamp((53-z)/43))
        direction=n if z>60 else (ankle-knee).normalized()
        rx,ry=sample_profile(95-z,[(0,8.2,7.8),(10,9.0,8.0),(25,7.8,6.9),(40,6.05,5.8),(48,6.1,5.6),(65,4.8,4.5),(73,4.8,4.4)])
        _,u,v=frame_axis(direction);points=[]
        for i in range(32):
            a=math.tau*i/32;fold=.30*math.sin(z*1.01+a*3)*gaussian(z,53,8)+.12*math.sin(z*.4-a*6)
            if abs(z-cut.z)<1e-5:fold=0
            points.append(center+u*((rx+fold)*math.cos(a))+v*((ry+fold)*math.sin(a)))
        owner=G['LegLeft'] if side=='r' and z<cut.z-1e-5 else core
        if owner!=last_owner and last is not None:
            assert side=='r';oldpts=[core.v[i].copy() for i in last]
            last=owner.ring(oldpts,rigid(thigh));cut_cap(owner,last,cut,n,thigh,3,True)
        ring=owner.ring(points,leg_weights)
        if last is not None:owner.bridge(last,ring,leg_material,(.10,.125,.13))
        if abs(z-cut.z)<1e-5 and side=='r':cut_cap(core,ring,cut,n,thigh,3,False)
        if last is None:owner.cap(ring,leg_material)
        last=ring;last_owner=owner
    owner=G['LegLeft'] if side=='r' else core
    owner.cap(last,leg_material,True)
    # Deformable leather shaft, rounded toe, layered sole, raised lacing.
    bootwf=lambda p,c=calf,f=foot:blend_weights(c,f,smooth((25-p.z)/13))
    horizontal_loft(owner,[(6,1,sign*9,7,5.3),(11,.1,sign*9,5.9,5.0),(18,-.1,sign*9,5.2,4.7),(24,-.2,sign*9,5.35,4.8)],'boot',bootwf,28)
    horizontal_loft(owner,[(1,5,sign*9,12.4,5.6),(3,5,sign*9,12.6,5.65),(6,5.7,sign*9,11.7,5.4),(9,4.1,sign*9,9.7,5.0),(11,.2,sign*9,5.65,4.8)],'boot',rigid(foot),32)
    horizontal_loft(owner,[(.5,5.2,sign*9,12.8,5.9),(2.0,5.2,sign*9,12.8,5.9)],'boot',rigid(foot),32,True,(.02,.019,.017))
    for z in (11,14,17,20):ribbon(owner,'Crossed_boot_lace_'+str(z),[(5.6,sign*9-2.1,z),(6.0,sign*9+2.1,z+1.5)],.40,.28,'boot',bootwf,(.12,.105,.08))
    owner.feature('Continuous_knee',rings=len(levels),weights='smooth thigh/calf window 62–44 cm')

# Reinforced jacket hem, offset placket, thick collar/lapels and sewn pockets.
horizontal_loft(core,[(98.7,-.6,0,10.5,15.1),(101.2,-.6,0,10.5,15.1)],'cloth',torso_weights,48,True,(.074,.10,.115))
for y in (() if LAB else (-.55,.55)):ribbon(core,'Front_placket',[(10.4,y,103),(10.6,y,114),(11.6,y,126),(10.8,y,137),(6.4,y,147)],.60,.16,'cloth',torso_weights,(.105,.135,.13))
def jacket_front(y,z,offset=.48):
    cx,rx,ry=sample_profile(z,torso_profile)
    return Vector((cx+rx*math.sqrt(max(.1,1-(y/ry)**2))+offset,y,z))
for y in ((-9,) if STAFF else (-9,9)):
    panel=[]
    for row in range(7):
        z=125+9*row/6
        panel.append(core.ring([jacket_front(y-3.2+6.4*j/6,z,.55+.18*math.sin(math.pi*row/6)) for j in range(7)],torso_weights))
    for row in range(6):
        for j in range(6):core.face((panel[row][j],panel[row][j+1],panel[row+1][j+1],panel[row+1][j]),'cloth',(.085,.135,.15))
    outline=[jacket_front(yy,zz,.64) for yy,zz in [(y-3.2,125),(y-3.2,134),(y+3.2,134),(y+3.2,125),(y-3.2,125)]]
    ribbon(core,'Pocket_outer_'+str(y),outline,.20,.18,'cloth',torso_weights,(.17,.195,.17))
    ribbon(core,'Pocket_flap_'+str(y),[jacket_front(y-3.4,134.0,.82),jacket_front(y,133.1,.86),jacket_front(y+3.4,134.0,.82)],1.8,.30,'cloth',torso_weights,(.115,.155,.17))
    core.feature('Curved_pocket_panel_'+str(y),kind='conforming_cloth_panel',grid=[7,7])
for sign in (-1,1):
    ribbon(core,'Folded_collar_'+str(sign),[(5.5,sign*2,144),(6.6,sign*6.5,149),(1,sign*6.8,153),(-3.9,sign*4.8,153)],3.0,.7,'cloth',torso_weights,(.13,.18,.185))
    ribbon(core,'Shoulder_sewn_line_'+str(sign),[(-1,sign*6,148),(0,sign*11,146.8),(0,sign*16,145.2)],.24,.08,'cloth',torso_weights,(.135,.155,.125))
    if args.variant=='maintenance':ribbon(core,'Faded_safety_strip_'+str(sign),[(11.1,sign*4,138),(10.3,sign*10,138),(7.0,sign*15,137)],.85,.065,'cloth',torso_weights,(.225,.235,.17))
# Thick turned collar wraps the back and sides, shortening the visible neck.
collar=[]
for j in range(25):
    a=math.radians(52+256*j/24)
    bottom=Vector((5.0*math.cos(a),5.4*math.sin(a),151.0))
    top=Vector((5.25*math.cos(a),5.7*math.sin(a),155.3+.4*(-math.cos(a))))
    collar.append(core.ring([bottom,top,top-Vector((.32*math.cos(a),.32*math.sin(a),.12)),bottom-Vector((.32*math.cos(a),.32*math.sin(a),0))],torso_weights))
for a,b in zip(collar,collar[1:]):core.bridge(a,b,'cloth',(.14,.185,.185))
core.cap(collar[0],'cloth');core.cap(collar[-1],'cloth',True)
# ID is a restrained worker cue, not special-enemy equipment.
ribbon(core,'Small_ID_card',[jacket_front(-8,129,.98),jacket_front(-8,133,.98)],3.1,.14,'cloth',rigid('spine_02'),(.32,.315,.215))
ribbon(core,'ID_marking',[jacket_front(-9.0,130,1.10),jacket_front(-7.0,130,1.10)],.20,.08,'hair',rigid('spine_02'))
if LAB:
    shirt=[]
    for z,width in [(117,.35),(126,2.1),(137,4.4),(146,1.8)]:
        shirt.append(core.ring([jacket_front(-width,z,.68),jacket_front(width,z,.68)],torso_weights))
    for a,b in zip(shirt,shirt[1:]):core.face((a[0],a[1],b[1],b[0]),'trouser')
    # Open hip-length coat tails end above the supported thigh sever plane.
    # Their lower left/right cloth weights follow their own proximal thigh.
    # No panel spans the two legs or a detachable boundary.
    for sign in (-1,1):
        panel=[]
        for j in range(15):
            f=j/14;row=[]
            for i in range(37):
                a=sign*(.22+(math.pi-.25)*i/36)
                hem=84.8+1.7*math.sin(3*a+.4)+.9*math.sin(7*a)
                z=hem+(107-hem)*f
                rx=11.3+2.2*(1-f)+.3*math.sin(a*7+f*5)
                ry=16.4+4.5*(1-f)
                row.append(Vector((-.6+rx*math.cos(a),ry*math.sin(a),z)))
            wf=lambda p,sign=sign:blend_weights('thigh_r' if sign>0 else 'thigh_l','pelvis',smooth((p.z-85)/17))
            panel.append(core.ring(row,wf))
        panel_faces=[]
        for j in range(14):
            for i in range(36):
                face=(panel[j][i],panel[j][i+1],panel[j+1][i+1],panel[j+1][i])
                face=face if sign>0 else tuple(reversed(face));core.face(face,'cloth');panel_faces.append(face)
        # Reverse-wound inset lining and consistently closed boundary edges
        # remain visible from inside an open, one-sided opaque coat.
        inside={i:core.vertex(core.v[i]-Vector((.16*core.v[i].x/13,.16*core.v[i].y/18,0)),core.w[i]) for row in panel for i in row}
        edges={}
        for face in panel_faces:
            core.face(tuple(inside[i] for i in reversed(face)),'cloth',(.18,.17,.14))
            for a,b in zip(face,face[1:]+face[:1]):edges.setdefault(tuple(sorted((a,b))),[]).append((a,b))
        for pairs in edges.values():
            if len(pairs)==1:
                a,b=pairs[0];core.face((b,a,inside[a],inside[b]),'cloth',(.18,.17,.14))
        ribbon(core,'Open_lab_lapel_'+str(sign),[jacket_front(sign*1.8,116,1.1),jacket_front(sign*7.5,135,1.1),jacket_front(sign*5.3,146,1.1)],4.7,.42,'cloth',torso_weights)
        ribbon(core,'Low_lab_pocket_'+str(sign),[jacket_front(sign*7,107,.95),jacket_front(sign*12,107,.95)],1.5,.30,'cloth',torso_weights)
    core.feature('Split_lab_coat_tails',minimum_hem_cm=82.2,supported_thigh_cut_cm=79.04,open_front=True,thickness_cm=.16)
elif STAFF:
    # Narrow work shirt with rolled cuffs and loose tie has an everyday silhouette.
    ribbon(core,'Loose_work_tie',[(6.8,0,147.5),(10.0,.5,140),(11.6,1.0,133),(11.0,2.4,120)],2.15,.35,'trouser',torso_weights,(.085,.065,.049))
    for z in (109,118,128,138):ellipsoid(core,'Shirt_button_'+str(z),jacket_front(-.35,z,.84),(.22,.27,.27),'bone','pelvis' if z<115 else 'spine_01' if z<128 else 'spine_02',12,6,accent=(.25,.23,.18))
    horizontal_loft(core,[(99.0,-.6,0,10.8,15.4),(100.6,-.6,0,10.8,15.4)],'boot',rigid('pelvis'),40)
    ribbon(core,'Worn_belt_buckle',[(10.8,-1.0,99.6),(10.8,1.2,99.6)],1.7,.25,'metal',rigid('pelvis'))
    core.feature('Staff_shirt_and_tie',rolled_sleeves=True,single_chest_pocket=True,waist_scale=.93)

# Neck and shared head seam: core's top ring deliberately uses head weights.
def neck_weights(p):return blend_weights('neck','head',smooth((p.z-153.3)/3.4))
neckrows=horizontal_loft(core,[(148.5,-.1,0,4.3,4.5),(151.0,0,0,4.1,4.4),(154.0,0,0,3.9,4.2),(157.8,0,0,4.,4.3)],'skin',neck_weights,96,False)
cut_cap(core,neckrows[-1],(0,0,157.8),(0,0,1),'head',4,False)
head=G['Head']; headrows=[];head_profile=[(157.8,0,4,4.3),(159,.35,4.8,5.2),(161,.45,5.55,5.9),(164,-.4,6.1,6.8),(167,-.8,6.9,7.1),(170,-1.2,7.1,7.5),(173,-1.5,7.2,7.7),(176,-1.9,6.8,7.3),(178.5,-1.9,5.5,6.2),(179.4,-1.9,4.8,5.4),(180.1,-1.8,3.8,4.3),(180.7,-1.7,2.5,2.8),(181,-1.7,.15,.18)]
HEAD_ROWS=73;HEAD_SIDES=96
for j in range(HEAD_ROWS):
    z=157.8+(181-157.8)*j/(HEAD_ROWS-1);cx,rx,ry=sample_profile(z,head_profile);points=[]
    for i in range(HEAD_SIDES):
        a=math.tau*i/HEAD_SIDES;x=cx+rx*math.cos(a);y=ry*math.sin(a)
        front=clamp((math.cos(a)-.15)/.7)
        nose=(2.3*gaussian(z,168.2,.9)+1.55*gaussian(z,170.2,1.8))*gaussian(y,-.18,1.22)
        nose+=.72*gaussian(z,167.57,.42)*gaussian(abs(y),.88,.43)
        brow=.78*gaussian(z,172.0,.6)*gaussian(abs(y),3.0,1.8)
        sockets=-1.35*gaussian(z,170.7,.95)*gaussian(abs(y),3.15,1.35)
        cheek=1.20*gaussian(z,168.6,.8)*gaussian(abs(y),5.0,1.4)-1.35*gaussian(z,166.5,1.4)*gaussian(abs(y),4.7,1.3)
        temple=-1.0*gaussian(z,173,2)*gaussian(abs(y),6.2,1)
        lip=.6*gaussian(z,164.8,1.2)*gaussian(y,0,3.1)
        jaw=.6*gaussian(z,162.5,.8)*gaussian(abs(y),4.2,1.3)
        x+=front*(nose+brow+sockets+cheek+temple+lip+jaw)
        # Fine forehead folds and nasolabial grooves belong to the skull mesh.
        x+=front*.10*math.sin((z-173)*15+.4*y)*gaussian(z,174,1.1)*gaussian(y,0,4)
        x-=front*.23*gaussian(abs(y),1.4+(168-z)*.40,.24)*gaussian(z,166.7,1.6)
        x-=front*.7*gaussian(y,4.4,1.3)*gaussian(z,166.3,2)
        x-=front*.95*gaussian(y,-3.9,1.25)*gaussian(z,163.0,1.75)
        y*=1+.035*math.sin(z*.41)*front
        if y<0:y*=1-.09*gaussian(z,162.0,2.0)
        if j==0:x=4*math.cos(a);y=4.3*math.sin(a)
        points.append(Vector((x,y,z)))
    headrows.append(head.ring(points,rigid('head')))
openings={key:{} for key in ('mouth','eye_left','eye_right','cheek')}
def opening_at(c):
    if c.x<2.7:return None
    if ((c.y+.18)/2.2)**2+((c.z-165.02-.12*c.y)/.78)**2<1:return 'mouth'
    for sign,key in [(-1,'eye_right'),(1,'eye_left')]:
        if ((c.y-sign*3.05)/1.42)**2+((c.z-(170.75+(.16 if sign<0 else -.22)))/.96)**2<1:return key
    if ((c.y+4.7)/.82)**2+((c.z-166.6+.16*c.y)/1.35)**2<1:return 'cheek'
    return None
for j in range(len(headrows)-1):
    for i in range(HEAD_SIDES):
        k=(i+1)%HEAD_SIDES;f=(headrows[j][i],headrows[j][k],headrows[j+1][k],headrows[j+1][i]);c=sum((head.v[v] for v in f),Vector())/4
        opening=opening_at(c)
        if opening:
            edges=openings[opening]
            for a,b in zip(f,f[1:]+f[:1]):key=tuple(sorted((a,b)));edges[key]=edges.get(key,0)+1
        else:head.face(f,'skin')
mouth=ordered_loop([e for e,n in openings['mouth'].items() if n==1],head);center=sum((head.v[i] for i in mouth),Vector())/len(mouth)
for i in mouth:
    p=head.v[i];a=math.atan2((p.z-165.02-.12*p.y)/.78,(p.y+.18)/2.2)
    p.y=-.18+2.2*math.cos(a);p.z=165.02+.12*p.y+.78*math.sin(a)
lip_roll=head.ring([Vector((head.v[i].x+(.20 if head.v[i].z>center.z else .29),center.y+(head.v[i].y-center.y)*.95,center.z+(head.v[i].z-center.z)*.94)) for i in mouth],rigid('head'))
head.bridge(mouth,lip_roll,'skin',(.34,.195,.135))
lip=head.ring([Vector((head.v[i].x+.06,center.y+(head.v[i].y-center.y)*.88,center.z+(head.v[i].z-center.z)*.77)) for i in mouth],rigid('head'))
head.bridge(lip_roll,lip,'skin',(.275,.12,.075))
gum=head.ring([Vector((4.35,center.y+(head.v[i].y-center.y)*.86,center.z+(head.v[i].z-center.z)*.82)) for i in mouth],rigid('head'))
head.bridge(lip,gum,'flesh',(.155,.035,.025))
inner=head.ring([Vector((3.0,center.y+(head.v[i].y-center.y)*.68,center.z+(head.v[i].z-center.z)*.67)) for i in mouth],rigid('head'))
head.bridge(gum,inner,'flesh',(.065,.014,.009));head.cap(inner,'hair',accent=(.015,.008,.007))
head.cap(headrows[-1],'skin');cut_cap(head,headrows[0],(0,0,157.8),(0,0,1),'head',4,True)
head.feature('Modeled_asymmetric_face',longitudinal_rings=HEAD_ROWS,circumference=HEAD_SIDES,features=['nose_bridge','brow','integrated_socket_lids','zygomatic_plane','temple_hollows','uneven_jaw','recessed_mouth_gums','open_cheek_wound'])
for sign in (-1,1):
    y=sign*3.05; z=170.75+(.16 if sign<0 else -.22)
    socket=ordered_loop([e for e,n in openings['eye_right' if sign<0 else 'eye_left'].items() if n==1],head)
    aperture=[]
    for index in socket:
        p=head.v[index];a=math.atan2((p.z-z)/.96,(p.y-y)/1.42)
        upper=math.sin(a)>0
        aperture.append(Vector((5.12+.09*math.sin(a),y+1.03*math.cos(a),z+(.35 if upper else .30)*math.sin(a)-(.11 if sign>0 and upper else 0))))
    roll=head.ring([head.v[index].lerp(p,.75)+Vector((.065,0,0)) for index,p in zip(socket,aperture)],rigid('head'))
    edge=head.ring(aperture,rigid('head'));head.bridge(socket,roll,'skin');head.bridge(roll,edge,'skin',(.35,.24,.19))
    wet=head.ring([p-Vector((.12,0,0)) for p in aperture],rigid('head'));head.bridge(edge,wet,'flesh',(.19,.065,.045))
    ellipsoid(head,'Recessed_eye_'+str(sign),(4.12,y,z),(.70,1.06,.67),'eye','head',24,12,accent=(.32,.29,.205))
    ellipsoid(head,'Clouded_iris_'+str(sign),(4.819,y+(.10 if sign<0 else -.07),z-.025),(.018,.245,.24),'eye','head',16,8,accent=(.105,.13,.08))
    # Ear helix and concha with a genuine inset dark cavity.
    ellipsoid(head,'Auricle_'+str(sign),(-.5,sign*7.35,169),(1.1,.60,2.20),'skin','head',20,12)
    pts=[]
    for j in range(25):
        a=math.tau*j/24;pts.append(Vector((-.1+.68*math.cos(a),sign*(7.8+.10*math.cos(a)),169+1.73*math.sin(a))))
    skin_tube(head,[(p,.23,.25) for p in pts],'skin',rigid('head'),8)
    ellipsoid(head,'Concha_'+str(sign),(.30,sign*7.89,168.8),(.54,.10,.86),'flesh','head',16,8,accent=(.17,.12,.095))
    head.feature('Integrated_nasal_ala_'+str(sign),kind='continuous_skull_form_and_recessed_underside_colour')
for i,y in enumerate((-1.55,-.78,-.10,.56,1.23)):
    z=165.48+.09*y;length=(.46,.64,.51,.23,.42)[i];x=5.25-.19*y*y
    horizontal_loft(head,[(z-length/2,x+.025,y,.14,.19),(z+length*.25,x,y,.18,.26),(z+length/2,x-.02,y,.15,.23)],'bone',rigid('head'),8,True,accent=(.38,.31,.16) if i in (0,3) else (.48,.39,.245))
    head.feature('Inset_tooth_'+str(i),kind='chipped_inset_volume')
# Deep cheek wound has an irregular skin rim and folded interior tissue.
wound=ordered_loop([e for e,n in openings['cheek'].items() if n==1],head)
for i in wound:
    p=head.v[i];a=math.atan2((p.z-166.6+.16*p.y)/1.35,(p.y+4.7)/.82)
    p.y=-4.7+.82*(1+.11*math.sin(a*5))*math.cos(a)
    p.z=166.6-.16*p.y+1.35*(1+.065*math.sin(a*7))*math.sin(a)
wc=sum((head.v[i] for i in wound),Vector())/len(wound)
wr=head.ring([wc+(head.v[i]-wc)*(.81+.05*math.sin(j*2.7))-Vector((.25,0,0)) for j,i in enumerate(wound)],rigid('head'))
head.bridge(wound,wr,'skin',(.235,.092,.055))
wi=head.ring([wc+(head.v[i]-wc)*.60-Vector((.80,0,0)) for i in wound],rigid('head'))
head.bridge(wr,wi,'flesh',(.145,.025,.016));head.cap(wi,'flesh',accent=(.060,.016,.010))
for j in range(3):
    zz=wc.z-.9+j*.61
    skin_tube(head,[(Vector((wc.x-.50,wc.y-.31,zz)),.06,.09),(Vector((wc.x-.37,wc.y+.09,zz+.24)),.12,.09),(Vector((wc.x-.60,wc.y+.29,zz+.48)),.04,.035)],'flesh',rigid('head'),8)
# A coherent receding side/back mass has curved irregular edges, rather than
# a rectangular patch. The higher left-side edge sweeps partway over the crown.
def hair_bounds(a):
    if STAFF:
        lower=171.0+3.8*max(0,math.cos(a))+.38*math.sin(7*a)+.22*math.sin(13*a)
        return lower,180.75-.25*math.cos(a*2)+.08*math.sin(11*a)
    if LAB:
        lower=166.9+4.0*abs(math.sin(a))+.45*math.sin(5*a)+.2*math.sin(12*a)
        upper=175.8+.75*math.sin(2*a+.6)+.15*math.sin(17*a)
        taper=smooth((a-1.05)/.38)*smooth((5.2-a)/.45);middle=(lower+upper)*.5
        return middle+(lower-middle)*taper,middle+(upper-middle)*taper
    lower=168.6+2.7*abs(math.sin(a))+.28*math.sin(8*a)+.16*math.sin(17*a+.7)
    upper=177.4+1.2*gaussian(a,4.15,.85)+.55*math.sin(a*2.1+.4)+.18*math.sin(13*a)
    taper=smooth((a-.88)/.43)*smooth((5.43-a)/.55)
    middle=(lower+upper)*.5
    return middle+(lower-middle)*taper,middle+(upper-middle)*taper

def scalp_point(a,z,lift=0.):
    cx,rx,ry=sample_profile(z,head_profile)
    p=Vector((cx+rx*math.cos(a),ry*math.sin(a),z))
    n=Vector((math.cos(a),math.sin(a),clamp((z-175)/5)*.75)).normalized()
    return p+n*lift

mass=[]
hair_start,hair_end=(.02,math.tau-.02) if STAFF else (1.07,5.18) if LAB else (.90,5.40)
hair_rows=33 if STAFF else 11
for j in range(hair_rows):
    t=j/(hair_rows-1);points=[]
    for i in range(81):
        a=hair_start+(hair_end-hair_start)*i/80;lo,hi=hair_bounds(a);z=lo+(hi-lo)*t
        ripple=.026*math.sin(a*37+t*9)*math.sin(math.pi*t)
        points.append(scalp_point(a,z,(.18 if STAFF else .035)+ripple))
    mass.append(head.ring(points,rigid('head')))
for j in range(hair_rows-1):
    for i in range(80):head.face((mass[j][i],mass[j][i+1],mass[j+1][i+1],mass[j+1][i]),'hair',(.065,.050,.032))

rng=random.Random(707031+(17 if LAB else 41 if STAFF else 0))
for index in range(84+160):
    fine=index>=84;a=rng.uniform(1.02 if args.variant=='maintenance' else hair_start+.11,hair_end-.15);lo,hi=hair_bounds(a)
    root_z=rng.uniform(lo+.15,hi-.1)
    length=rng.uniform(1.0,3.5) if not fine else rng.uniform(.9,3.1)
    if LAB:length*=1.35
    if STAFF:length*=.75
    sweep=rng.uniform(.10,.23);curl=rng.uniform(.035,.13);phase=rng.uniform(0,math.tau)
    thickness=rng.uniform(.10,.21) if not fine else rng.uniform(.025,.045)
    points=[];count=6 if not fine else 4
    for j in range(count):
        t=j/(count-1);az=a+(math.pi-a)*sweep*t+.021*math.sin(t*5+phase)*math.sin(math.pi*t)
        z=root_z-length*t
        lift=(.20 if STAFF else .042)+(rng.uniform(.10,.24) if j==count//2 else .12)*math.sin(math.pi*t)
        if STAFF:lift+=.12*math.sin(math.pi*t)**2;az+=.065*math.sin(t*7+phase)*math.sin(math.pi*t)
        p=scalp_point(az,z,lift)
        # Broad flattened locks overlap; thin strands follow that same sweep.
        radius=thickness*(.22+.95*math.sin(math.pi*t))*(1-.87*t)
        points.append((p,max(.007,radius),max(.006,radius*(.58 if not fine else .88))))
    shade=rng.uniform(.83,1.18)
    skin_tube(head,points,'hair',rigid('head'),6 if not fine else 4,accent=tuple(v*shade for v in (.064,.048,.029)))
head.feature('Irregular_swept_hair',modeled_locks=84,fine_strands=160,opaque=True,scalp_surface='curved receding side/back coverage with irregular edge and partial crown sweep',placement='deterministic continuous sampling; no grid of roots')

# Sparse eyebrows sit on the actual brow surface, adding a human facial cue.
def brow_surface(y,z):
    row=min(headrows,key=lambda ids:abs(head.v[ids[0]].z-z))
    front=sorted((head.v[i] for i in row if head.v[i].x>1),key=lambda p:p.y)
    for a,b in zip(front,front[1:]):
        if a.y<=y<=b.y:
            f=(y-a.y)/max(.0001,b.y-a.y)
            return Vector((a.x*(1-f)+b.x*f+.035,y,z))
    return min(front,key=lambda p:abs(p.y-y)).copy()
for sign in (-1,1):
    for strand in range(11):
        u=(strand+.35*rng.random())/11;y=sign*(1.90+2.5*u);z=172.2+.28*math.sin(math.pi*u)
        a=brow_surface(y,z);b=brow_surface(y+sign*.29,z+.10)
        skin_tube(head,[(a,.036,.026),(a.lerp(b,.5)+Vector((.018,0,.02)),.043,.031),(b,.006,.006)],'hair',rigid('head'),5,accent=(.10,.07,.043))
head.feature('Eyebrows',modeled_strands=22,attached_to='sampled original brow surface')
if args.variant!='maintenance':
    # Deform the complete face assembly together: eyelid rims, eyes, teeth,
    # wound, ears and hair stay registered to their skull, not floating props.
    for p in head.v:
        if LAB:p.y=-p.y
        influence=smooth((p.z-157.8)/3.0)
        if LAB:
            p.y*=1-influence*(.055+.075*gaussian(p.z,162.5,2.8))
            p.x-=influence*(.35*gaussian(p.z,164.0,2.4)+.18*gaussian(p.z,173,2.3))*clamp(p.x/5)
            p.y+=influence*.24*gaussian(p.z,168,4)
        else:
            p.y*=1+influence*.055*gaussian(p.z,162.7,2.6)
            p.x+=influence*.28*gaussian(p.z,161.4,1.5)*clamp(p.x/5)
            p.x-=influence*.20*gaussian(p.y,3.5,1.3)*gaussian(p.z,170.0,2.8)
    if LAB:head.f=[tuple(reversed(face)) for face in head.f]
    head.feature('Variant_facial_structure',variant=VARIANT,method='Complete modeled face deformation with unchanged neck rim',form='narrow jaw and cheek plane, asymmetric shifted midface' if LAB else 'broader lower jaw and chin, asymmetric eye plane')
# A rounded adult cranium, preserving the neck/cut location and bind matrices.
for p in head.v:
    if p.z>171:p.z=171+(p.z-171)*.76
head_skin_vertices={i for face,key in zip(head.f,head.mat) if key=='skin' for i in face}
head_center=Vector((-.2,0,168.7));head_margin=[]
for i in head_skin_vertices:
    p=head.v[i];closest=Vector((head_center.x,head_center.y,clamp(p.z,165.4,172.0)))
    head_margin.append(9.-(p-closest).length)
assert min(head_margin)>=-.001, ('Variant head exceeds the established query',VARIANT,min(head_margin))

# Create the actual five meshes and preserve identical exterior seam normals.
objects={part:g.object() for part,g in G.items()}
for ob in objects.values():
    ob.data.validate(verbose=False);ob.data.update();ob.data.calc_loop_triangles()
normal_overrides={part:{} for part in objects}
def exterior_normal(part,index):
    result=Vector()
    for polygon,key in zip(objects[part].data.polygons,G[part].mat):
        if index in polygon.vertices and key in ('skin','cloth','trouser'):
            result+=polygon.normal*polygon.area
    assert result.length>1e-8
    return result.normalized()
for sid,part in [(1,'ArmLeft'),(2,'ArmRight'),(3,'LegLeft'),(4,'Head')]:
    a=objects['Core'];b=objects[part];ia=G['Core'].seams[sid];ib=G[part].seams[sid]
    assert len(ia)==len(ib)
    # The exterior smooth normal ignores recessed cap faces, then is shared.
    for aa in ia:
        bb=min(ib,key=lambda j:(a.data.vertices[aa].co-b.data.vertices[j].co).length)
        assert (a.data.vertices[aa].co-b.data.vertices[bb].co).length<.001
        normal=(exterior_normal('Core',aa)+exterior_normal(part,bb)).normalized()
        normal_overrides['Core'][aa]=normal;normal_overrides[part][bb]=normal
for part,ob in objects.items():
    normals=[n.vector.copy() for n in ob.data.corner_normals]
    for polygon,key in zip(ob.data.polygons,G[part].mat):
        if key not in ('skin','cloth','trouser'):continue
        for li in polygon.loop_indices:
            vi=ob.data.loops[li].vertex_index
            if vi in normal_overrides[part]:normals[li]=normal_overrides[part][vi]
    ob.data.normals_split_custom_set(normals)

def reset_pose():
    rig.animation_data_clear()
    for bone in rig.pose.bones:bone.matrix_basis=Matrix.Identity(4)
    scene.frame_set(1);bpy.context.view_layer.update()
def turn_bone(name,axis,degrees):
    bone=rig.pose.bones[name];headpos=bone.head.copy()
    bone.matrix=Matrix.Translation(headpos)@Matrix.Rotation(math.radians(degrees),4,Vector(axis))@Matrix.Translation(-headpos)@bone.matrix
    bpy.context.view_layer.update()
def seam_sample(label):
    deps=bpy.context.evaluated_depsgraph_get();result=[]
    for sid,part in [(1,'ArmLeft'),(2,'ArmRight'),(3,'LegLeft'),(4,'Head')]:
        a=objects['Core'].evaluated_get(deps).data;b=objects[part].evaluated_get(deps).data
        ia=G['Core'].seams[sid];ib=G[part].seams[sid]
        gap=max(min((a.vertices[i].co-b.vertices[j].co).length for j in ib) for i in ia)
        assert gap<.001,(label,part,gap)
        result.append({'pose':label,'part':part,'rim_vertices':len(ia),'max_gap_cm':gap})
    return result
samples=seam_sample('bind')
for label,changes in [('neck_turn',[('head',(0,0,1),35),('neck',(0,1,0),18)]),('arm_flex',[('upperarm_r',(0,1,0),-55),('lowerarm_r',(0,1,0),-70),('upperarm_l',(0,0,1),25)]),('knee_flex',[('thigh_r',(0,1,0),-45),('calf_r',(0,1,0),75)]),('torso_twist',[('spine_01',(0,0,1),22),('spine_02',(0,1,0),15)])]:
    reset_pose()
    for change in changes:turn_bone(*change)
    samples+=seam_sample(label)
for name in ('A_Infected_Walk','A_Infected_Run','A_Infected_Attack','A_Infected_AttackOneArm','A_Infected_Hit','A_Infected_Death'):
    action=bpy.data.actions.get(name)
    if not action:continue
    reset_pose();rig.animation_data_create();rig.animation_data.action=action
    if len(action.slots):rig.animation_data.action_slot=action.slots[0]
    lo,hi=action.frame_range
    for phase in (0,.25,.48,.75,1):
        value=lo+(hi-lo)*phase;scene.frame_set(math.floor(value),subframe=value-math.floor(value));bpy.context.view_layer.update()
        samples+=seam_sample(name+'/'+str(phase))
reset_pose()
validation={'status':'SOURCE_NUMERIC_PASS_PENDING_VISUAL_AND_GAMEPLAY_REVIEW','rig_source':CONFIG['rig_source'],
    'bind_matrices_unchanged':all(max(abs(REST[b.name][i][j]-b.matrix_local[i][j]) for i in range(4) for j in range(4))<1e-8 for b in rig.data.bones),
    'bone_count':len(REST),'seam_samples':samples,'meshes':{},'source_inputs_unchanged':{},
    'limit':'Finite weights, exact evaluated seams and source bounds do not prove visual quality, motion, query/physics fit or gameplay.'}
validation['head_query_fit']={'component_center_cm':list(head_center),'radius_cm':9.,'half_height_cm':12.3,'skin_vertices':len(head_skin_vertices),'minimum_margin_cm':min(head_margin),'cosmetic_hair_excluded':True,'scope':'Bind-pose actual modeled skin envelope; runtime evaluated fit remains separate.'}
for part,ob in objects.items():
    totals=[sum(w.weight for w in v.groups) for v in ob.data.vertices]
    assert min(totals)>.9999 and max(totals)<1.0001
    assert all(math.isfinite(c) for v in ob.data.vertices for c in v.co)
    assert all(math.isfinite(c) for poly in ob.data.polygons for c in poly.normal)
    validation['meshes'][part]={'vertices':len(ob.data.vertices),'triangles':len(ob.data.loop_triangles),'weight_sum_min':min(totals),'weight_sum_max':max(totals),'materials':[m.name for m in ob.data.materials],'features':G[part].features}
validation['material_graphs']={}
for key,m in MATS.items():
    nodes=m.node_tree.nodes;p=nodes.get('Principled BSDF')
    assert p.inputs['Base Color'].is_linked and p.inputs['Base Color'].links[0].from_node.type=='VERTEX_COLOR'
    assert p.inputs['Roughness'].is_linked and p.inputs['Roughness'].links[0].from_node.type=='VERTEX_COLOR'
    colors=[c.color for ob in objects.values() for poly in ob.data.polygons if ob.data.materials[poly.material_index]==m for li in poly.loop_indices for c in [ob.data.color_attributes['Color'].data[li]]]
    detail_key='cloth' if key=='trouser' else key
    if colors:validation['material_graphs'][m.name]={'vertex_color_node':'Color','rgb_min':[min(c[i] for c in colors) for i in range(3)],'rgb_max':[max(c[i] for c in colors) for i in range(3)],'roughness_min':min(c[3] for c in colors),'roughness_max':max(c[3] for c in colors),'normal_texture':DETAIL_TEXTURES[detail_key].name if detail_key in DETAIL_TEXTURES else None}
assert sum(d['triangles'] for d in validation['meshes'].values())<CONFIG['provisional_triangle_budget']
for path,before in GUARDS.items():
    assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==before
    validation['source_inputs_unchanged'][path]=before

def export(ob):
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);ob.select_set(True);bpy.context.view_layer.objects.active=rig
    path=EXPORT/(ob.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE','MESH'},axis_forward='-Y',axis_up='Z',global_scale=1,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',use_space_transform=True,bake_space_transform=False,add_leaf_bones=False,primary_bone_axis='Y',secondary_bone_axis='X',use_armature_deform_only=False,mesh_smooth_type='FACE',use_mesh_modifiers=True,bake_anim=False,path_mode='AUTO',colors_type='LINEAR')
    tree,_=parse_fbx.parse(str(path));exported=[]
    def visit(node):
        if node.id==b'Colors':exported.extend(node.props[0])
        for child in node.elems:visit(child)
    visit(tree)
    assert exported and len(exported)%4==0
    source_colors=[c.color[:] for c in ob.data.color_attributes['Color'].data]
    expected_min=[min(c[i] for c in source_colors) for i in range(4)];expected_max=[max(c[i] for c in source_colors) for i in range(4)]
    actual_min=[min(exported[i::4]) for i in range(4)];actual_max=[max(exported[i::4]) for i in range(4)]
    assert max(abs(a-b) for a,b in zip(expected_min+expected_max,actual_min+actual_max))<1e-6
    return {'source':path.relative_to(ROOT).as_posix(),'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'exported_vertex_colors':{'space':'LINEAR','rgba_records':len(exported)//4,'minimum':actual_min,'maximum':actual_max,'source_range_match':True}}
inventory={'candidate':'07','variant':args.variant,'variant_id':VARIANT,'display_name':CONFIG['display_name'],'skeleton_asset':CONFIG['skeleton_asset'],'editable_source':'ArtSource/Characters/Candidate07/'+VARIANT+'.blend','anatomy':CONFIG['anatomy'],'fbx_vertex_color_space':'LINEAR: Unreal skeletal FBX importer directly quantizes these RGB/alpha values','meshes':{},'materials':{},'textures':{},'cuts':{},'validation_limit':validation['limit']}
# FBX carries geometry, vertex colour and slots only. Dedicated material import
# binds original normal maps, avoiding absolute texture paths in the FBX.
image_nodes=[(n,n.image) for m in MATS.values() for n in m.node_tree.nodes if n.type=='TEX_IMAGE']
for n,_ in image_nodes:n.image=None
for part,ob in objects.items():inventory['meshes'][part]={**validation['meshes'][part],**export(ob),'asset':'/Game/ONE/Characters/Candidate07/'+ob.name}
for n,image in image_nodes:n.image=image
for key,image in DETAIL_TEXTURES.items():
    path=SOURCE/(image.name+'.png')
    inventory['textures'][image.name]={'source':path.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/Textures/Candidate07/'+image.name,'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'size':[256,256],'kind':'original periodic tangent-space normal','green_convention':'OpenGL; Unreal flip green channel','external_inputs':False}
    image.filepath='//'+image.name+'.png'
for key,(name,color,rough,metal) in PALETTE.items():
    detail_key='cloth' if key=='trouser' else key
    inventory['materials'][name]={'asset':'/Game/ONE/Materials/Candidate07/'+name,'vertex_color':'RGB actual authored base colour; Alpha roughness','metallic':metal,'fallback_roughness':rough,'normal_texture':DETAIL_TEXTURES[detail_key].name if detail_key in DETAIL_TEXTURES else None}
old=json.loads((ROOT/'ArtSource/Characters/Candidate03/infected_inventory.json').read_text())
for part in ('Head','ArmLeft','ArmRight','LegLeft'):
    entry=old['cuts'][part];inventory['cuts'][part]={k:entry[k] for k in ('bone','source_component_cm','ue_component_cm')}
    inventory['cuts'][part]['paired_rim_weights']='Identical across split; retain proximal bone and terminate removed physics chains.'
for screen in bpy.data.screens:
    for area in screen.areas:
        for space in area.spaces:
            if space.type=='FILE_BROWSER' and space.params:space.params.directory=b'//';space.params.filename=''
scene.render.filepath='//'+VARIANT+'_neutral.png';scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/(VARIANT+'.blend')),compress=True,relative_remap=False)
for image in DETAIL_TEXTURES.values():
    resolved=Path(bpy.path.abspath(image.filepath)).resolve()
    assert resolved==(SOURCE/(image.name+'.png')).resolve() and resolved.is_file(),resolved
    image.reload()
(SOURCE/(args.variant+'_inventory.json')).write_text(json.dumps(inventory,indent=2)+'\n')
(SOURCE/(args.variant+'_validation.json')).write_text(json.dumps(validation,indent=2)+'\n')
for path,before in GUARDS.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==before,path
print('C07_'+VARIANT.upper()+'_SOURCE',json.dumps({'triangles':sum(d['triangles'] for d in validation['meshes'].values()),'seam_samples':len(samples),'output':'ArtSource/Characters/Candidate07'}),flush=True)

if args.render:
    scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.threads_mode='FIXED';scene.render.threads=8
    scene.render.resolution_x=1400;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
    scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.13,.145,.16,1);scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.65
    scene.view_settings.view_transform='AgX'
    data=bpy.data.cameras.new('C07_inspection_camera');camera=bpy.data.objects.new('C07_inspection_camera',data);scene.collection.objects.link(camera);scene.camera=camera;camera.data.type='ORTHO'
    for name,location,energy,size in [('Key',(190,-250,310),1700,200),('Fill',(130,230,230),1100,200),('Rim',(-160,20,280),1200,170)]:
        ld=bpy.data.lights.new('C07_'+name,'AREA');ld.energy=energy*300;ld.shape='DISK';ld.size=size
        lo=bpy.data.objects.new('C07_'+name,ld);scene.collection.objects.link(lo);lo.location=location;lo.rotation_euler=(Vector((0,0,100))-lo.location).to_track_quat('-Z','Y').to_euler()
    for label,location,target,scale in [('neutral',(300,-430,255),(0,0,91),270),('face',(250,-230,210),(0,0,165),45),('side',(25,-450,185),(0,0,91),270),('flex',(300,-430,255),(0,0,100),280),('cuts',(310,-450,260),(0,0,106),290)]:
        reset_pose()
        if label=='flex':
            for change in [('upperarm_r',(0,1,0),-55),('lowerarm_r',(0,1,0),-70),('upperarm_l',(0,0,1),25),('thigh_r',(0,1,0),-35),('calf_r',(0,1,0),65),('head',(0,0,1),25)]:turn_bone(*change)
        if label=='cuts':
            for part,offset in [('Head',(0,0,26)),('ArmLeft',(0,32,0)),('ArmRight',(0,-32,0)),('LegLeft',(0,26,-12))]:objects[part].location=offset
        camera.location=location;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale
        scene.render.filepath=str(SOURCE/(VARIANT+'_'+label+'.png'));bpy.ops.render.render(write_still=True)
    print('C07_SOURCE_PREVIEWS_DONE actual source stills; no gameplay acceptance',flush=True)
