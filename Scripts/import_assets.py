"""Run in the full UnrealEditor with ProjectONE.uproject
 -ExecutePythonScript="<absolute path to this script>" -unattended -nop4 -nosplash.
StaticMeshEditorSubsystem is required for deterministic environment collision.
The -run=pythonscript commandlet does not initialize it in this UE5.7 installation.
For unattended use, a full-editor wrapper can run this script and request editor
shutdown after successful completion; all repository paths derive from __file__.
Imports ONLY Project ONE's original exports, creates explicit native materials,
and preserves original Blender material-slot names. Safe to rerun for iteration.
"""
import unreal as u
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPORTS = ROOT/'ArtSource'/'Exports'
TOOLS=u.AssetToolsHelpers.get_asset_tools()
LIB=u.EditorAssetLibrary
MEL=u.MaterialEditingLibrary
LOG=[]

def setp(obj, name, value):
    obj.set_editor_property(name,value)
def optional(obj,name,value):
    try: setp(obj,name,value)
    except Exception as ex: u.log_warning(f'Optional {name}: {ex}')
def log(s):
    LOG.append(str(s)); u.log('ONE IMPORT: '+str(s))
def task_for(file,dest,options=None,factory=None):
    # Legacy reimport reads stored FBX flags ahead of the task's options.
    existing=LIB.load_asset(dest+'/'+file.stem) if LIB.does_asset_exist(dest+'/'+file.stem) else None
    if existing and file.suffix.lower()=='.fbx':
        data=existing.get_editor_property('asset_import_data')
        setp(data,'force_front_x_axis',False)
        setp(data,'convert_scene',True);setp(data,'convert_scene_unit',True)
        if isinstance(existing,u.SkeletalMesh):
            setp(data,'update_skeleton_reference_pose',not file.stem.endswith(('_Head','_ArmL')))
            setp(data,'vertex_color_import_option',u.VertexColorImportOption.REPLACE)
        if isinstance(existing,u.AnimSequence):
            setp(data,'use_default_sample_rate',False);setp(data,'custom_sample_rate',30)
        LIB.save_loaded_asset(existing)
    t=u.AssetImportTask(); t.filename=str(file); t.destination_path=dest
    t.destination_name=file.stem; t.automated=True; t.replace_existing=True
    t.replace_existing_settings=True; t.save=True
    if options: t.options=options
    if factory: t.factory=factory
    TOOLS.import_asset_tasks([t])
    result=[LIB.load_asset(p) for p in t.imported_object_paths]
    log(f'{file.name} -> {t.imported_object_paths}')
    if not result: raise RuntimeError('No imported objects: '+str(file))
    return result

u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
for folder in ('Materials','Characters','Animations','Art/Environment','Audio','Textures'):
    LIB.make_directory('/Game/ONE/'+folder)

for file in sorted((ROOT/'ArtSource'/'Textures').glob('*.png')):
    imported=task_for(file,'/Game/ONE/Textures',factory=u.TextureFactory())
    for tex in imported:
        if isinstance(tex,u.Texture2D):
            tex.set_editor_property('srgb',False)
            LIB.save_loaded_asset(tex)

palette=json.loads((ROOT/'ArtSource'/'Environment'/'manifest.json').read_text())['palette']
# Character palette is also authored here explicitly if the character manifest
# is unavailable during the first independent environment import.
palette.update({
 'M_UniformSlate': [[.06,.095,.105,1],.87,.0],
 'M_Ceramic': [[.37,.405,.365,1],.75,.15],
 'M_Webbing': [[.025,.040,.039,1],.98,.0],
 'M_BootRubber': [[.014,.018,.020,1],.95,.0],
 'M_Skin': [[.46,.285,.19,1],.8,.0],
 'M_Hair': [[.025,.019,.015,1],.92,.0],
 'M_OrangeFabric': [[.35,.125,.036,1],.93,.0],
 'M_InfectedSkin': [[.40,.36,.26,1],.89,.0],
 'M_Gore': [[.18,.009,.015,1],.77,.0],
 'M_IDBadge': [[.72,.73,.63,1],.83,.0],
 'M_Visor': [[.014,.055,.065,1],.32,.35],
})
char_palette=ROOT/'ArtSource'/'Characters'/'materials.json'
if char_palette.exists():
    for name,spec in json.loads(char_palette.read_text()).items():
        palette[name]=[spec['color'],spec['roughness'],spec['metallic']]

def scalar(mat,value,x,y):
    n=MEL.create_material_expression(mat,u.MaterialExpressionConstant,x,y)
    n.set_editor_property('r',float(value)); return n
def rgb(mat,color,x=-350,y=0):
    n=MEL.create_material_expression(mat,u.MaterialExpressionConstant3Vector,x,y)
    n.set_editor_property('constant',u.LinearColor(*color[:3],1)); return n
def new_material(name):
    p='/Game/ONE/Materials/'+name
    m=LIB.load_asset(p) if LIB.does_asset_exist(p) else TOOLS.create_asset(name,'/Game/ONE/Materials',u.Material,u.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    return m
materials={}
for name,(color,rough,metal) in palette.items():
    m=new_material(name)
    base=rgb(m,color,-600,0)
    vertex=MEL.create_material_expression(m,u.MaterialExpressionVertexColor,-600,-170)
    tint=MEL.create_material_expression(m,u.MaterialExpressionMultiply,-300,0)
    MEL.connect_material_expressions(base,'',tint,'A')
    MEL.connect_material_expressions(vertex,'RGB',tint,'B')
    MEL.connect_material_property(tint, '',u.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar(m,rough,-350,180),'',u.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(m,metal,-350,290),'',u.MaterialProperty.MP_METALLIC)
    if name.startswith('M_Light'):
        MEL.connect_material_property(rgb(m,[c*2.7 for c in color[:3]],-350,-150),'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    optional(m,'used_with_skeletal_mesh',True)
    MEL.recompile_material(m); LIB.save_loaded_asset(m)
    materials[name]=m

blood=new_material('M_Blood')
blood.set_editor_property('material_domain',u.MaterialDomain.MD_DEFERRED_DECAL)
blood.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
MEL.connect_material_property(rgb(blood,[.14,.004,.009]),'',u.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(scalar(blood,.68,-350,200),'',u.MaterialProperty.MP_ROUGHNESS)
mask=LIB.load_asset('/Game/ONE/Textures/T_BloodMask')
if mask:
    sample=MEL.create_material_expression(blood,u.MaterialExpressionTextureSample,-500,380)
    sample.texture=mask
    sample.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    MEL.connect_material_property(sample,'A',u.MaterialProperty.MP_OPACITY)
MEL.recompile_material(blood);LIB.save_loaded_asset(blood)
flash=new_material('M_Muzzle')
flash.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
flash.set_editor_property('two_sided',True)
MEL.connect_material_property(rgb(flash,[16,6.2,.65]),'',u.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(flash);LIB.save_loaded_asset(flash)

def fbx_options(kind,skeleton=None):
    o=u.FbxImportUI(); o.automated_import_should_detect_type=False
    o.import_materials=False; o.import_textures=False
    o.import_as_skeletal=kind!='static'
    o.import_mesh=kind!='animation'
    o.import_animations=kind=='animation'
    o.create_physics_asset=False
    if skeleton: o.skeleton=skeleton
    if kind=='static':
        o.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
        d=o.static_mesh_import_data
        setp(d,'combine_meshes',True);setp(d,'auto_generate_collision',True)
        setp(d,'generate_lightmap_u_vs',False)
    elif kind=='animation':
        o.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION
        d=o.anim_sequence_import_data
        setp(d,'animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        setp(d,'use_default_sample_rate',False);setp(d,'custom_sample_rate',30)
        setp(d,'remove_redundant_keys',False)
    else:
        o.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH
        d=o.skeletal_mesh_import_data
        setp(d,'import_morph_targets',False);setp(d,'use_t0_as_ref_pose',False)
        setp(d,'update_skeleton_reference_pose',skeleton is None)
    # Actual UE audit: False retains Blender +X forward; True rotates it to -Y.
    setp(d,'convert_scene',True);setp(d,'convert_scene_unit',True);setp(d,'force_front_x_axis',False)
    if kind!='animation':
        setp(d,'normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        setp(d,'vertex_color_import_option',u.VertexColorImportOption.REPLACE)
    return o

def assign_materials(asset):
    if isinstance(asset,u.StaticMesh):
        for i,slot in enumerate(asset.static_materials):
            name=str(slot.material_slot_name)
            if name in materials: asset.set_material(i,materials[name])
            else: log('UNMAPPED MATERIAL '+name+' on '+asset.get_name())
        environment_kit={
            'SM_FloorModule','SM_WallBay','SM_CutawayBarrier','SM_PressureDoor',
            'SM_PowerRack','SM_PressureVessel','SM_LabConsole','SM_ResearchBench',
        }
        if asset.get_path_name().startswith('/Game/ONE/Art/Environment/') and asset.get_name() in environment_kit:
            # FBX reimport can preserve old, differently oriented hulls. Replace
            # the kit's collision from its current render geometry on every import.
            editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
            if editor is None:
                raise RuntimeError('StaticMeshEditorSubsystem is unavailable. Run the full UnrealEditor with '
                                   '-ExecutePythonScript, not UnrealEditor-Cmd -run=pythonscript.')
            if not editor.remove_collisions(asset):
                raise RuntimeError('Could not remove previous environment collision: '+asset.get_name())
            hull=editor.add_simple_collisions(asset,u.ScriptCollisionShapeType.NDOP18)
            if hull!=0 or editor.get_convex_collision_count(asset)!=1 or editor.get_simple_collision_count(asset)!=0:
                raise RuntimeError('Expected one fresh NDOP18 environment hull: '+asset.get_name())
            log('REBUILT COLLISION '+asset.get_name()+' current-render NDOP18 hull=1')
        # Full detail single LOD, sufficient for this small authored kit.
        LIB.save_loaded_asset(asset)
    elif isinstance(asset,u.SkeletalMesh):
        slots=asset.materials
        for slot in slots:
            name=str(slot.material_slot_name)
            if name in materials: slot.material_interface=materials[name]
            else: log('UNMAPPED MATERIAL '+name+' on '+asset.get_name())
        asset.materials=slots
        LIB.save_loaded_asset(asset)

for file in sorted(EXPORTS.glob('SM_*.fbx')):
    dest='/Game/ONE/Characters' if file.stem.startswith('SM_Infected') else '/Game/ONE/Art/Environment'
    for asset in task_for(file,dest,fbx_options('static'),u.FbxFactory()): assign_materials(asset)

skeletons={}
for base in ('SK_Response','SK_Infected','SK_Infected_Head','SK_Infected_ArmL'):
    file=EXPORTS/(base+'.fbx')
    if not file.exists():
        log('NOT YET EXPORTED: '+str(file));continue
    skel=skeletons.get('Infected') if base in ('SK_Infected_Head','SK_Infected_ArmL') else None
    results=task_for(file,'/Game/ONE/Characters',fbx_options('skeletal',skel),u.FbxFactory())
    mesh=next(a for a in results if isinstance(a,u.SkeletalMesh))
    assign_materials(mesh)
    if base in ('SK_Response','SK_Infected'):
        skeletons[base.replace('SK_','')]=mesh.skeleton
    log('SKELETAL '+base+' skeleton '+mesh.skeleton.get_name())

for file in sorted(EXPORTS.glob('A_*.fbx')):
    who='Response' if file.stem.startswith('A_Response') else 'Infected'
    skeleton=skeletons.get(who)
    if not skeleton:
        mesh=LIB.load_asset('/Game/ONE/Characters/SK_'+who)
        if mesh: skeleton=mesh.skeleton
    if not skeleton: raise RuntimeError('Animation without skeleton: '+file.name)
    assets=task_for(file,'/Game/ONE/Animations',fbx_options('animation',skeleton),u.FbxFactory())
    for a in assets:
        if isinstance(a,u.AnimSequence):
            target='/Game/ONE/Animations/'+file.stem
            if a.get_name()!=file.stem:
                if LIB.does_asset_exist(target): LIB.delete_asset(target)
                LIB.rename_asset(a.get_path_name(),target)
            setp(a,'enable_root_motion',False)
            LIB.save_loaded_asset(a)
            log('ANIMATION '+file.stem+' duration '+str(a.get_play_length()))

for file in sorted((ROOT/'ArtSource'/'Audio').glob('*.wav')):
    task_for(file,'/Game/ONE/Audio',factory=u.SoundFactory())
LIB.save_directory('/Game/ONE',only_if_is_dirty=False,recursive=True)
if LIB.does_directory_exist('/Game/ONE/AxisProbe'):
    LIB.delete_directory('/Game/ONE/AxisProbe')
(ROOT/'Evidence'/'asset_import_log.txt').write_text('\n'.join(LOG),encoding='utf-8')
log('IMPORT COMPLETE')
