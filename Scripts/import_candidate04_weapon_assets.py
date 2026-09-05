"""Targeted C04 weapon/animation import; root coordinates the Unreal window.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript -script=this_script
 -unattended -nop4 -AllowCommandletRendering -NoSound
No audio is imported here. No C01-C03 mesh, skeleton, animation or material is rebuilt.
Run sanitize_asset_metadata.py on the new source files before the final import.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
INVENTORY=json.loads((ROOT/'ArtSource/Weapons/Candidate04/inventory.json').read_text(encoding='utf-8'))
LIB=u.EditorAssetLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()
MEL=u.MaterialEditingLibrary
REPORT={'candidate':'04','scope':'Only inventory-listed original weapon assets and upper-body clips','materials':{},'static_meshes':{},'animations':{}}
MATERIAL_DEST='/Game/ONE/Materials/Candidate04'
MESH_DEST='/Game/ONE/Art/Weapons/Candidate04'
ANIM_DEST='/Game/ONE/Animations/Candidate04'

def setp(obj,name,value):obj.set_editor_property(name,value)
def require(path):
    obj=LIB.load_asset(path)
    if obj is None:raise RuntimeError('Required asset unavailable: '+path)
    return obj
def connect(node,output,property):
    if not MEL.connect_material_property(node,output,property):raise RuntimeError('Material property connection failed '+str(property))
def wire(a,output,b,input):
    if not MEL.connect_material_expressions(a,output,b,input):raise RuntimeError('Material expression connection failed '+input)

for directory in (MATERIAL_DEST,MESH_DEST,ANIM_DEST):LIB.make_directory(directory)
for name,values in INVENTORY['materials'].items():
    path=MATERIAL_DEST+'/'+name
    mat=LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(name,MATERIAL_DEST,u.Material,u.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    base=MEL.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-600,0)
    setp(base,'constant',u.LinearColor(*values['base_color']))
    vertex=MEL.create_material_expression(mat,u.MaterialExpressionVertexColor,-600,150)
    multiply=MEL.create_material_expression(mat,u.MaterialExpressionMultiply,-340,0)
    wire(base,'',multiply,'A');wire(vertex,'',multiply,'B');connect(multiply,'',u.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(values['roughness'],u.MaterialProperty.MP_ROUGHNESS,280),(values['metallic'],u.MaterialProperty.MP_METALLIC,380)]:
        scalar=MEL.create_material_expression(mat,u.MaterialExpressionConstant,-340,y);setp(scalar,'r',value);connect(scalar,'',prop)
    if values['emissive_gain']:
        gain=MEL.create_material_expression(mat,u.MaterialExpressionScalarParameter,-580,-170)
        setp(gain,'parameter_name','EnergyGain');setp(gain,'default_value',values['emissive_gain'])
        emission=MEL.create_material_expression(mat,u.MaterialExpressionMultiply,-320,-180)
        wire(base,'',emission,'A');wire(gain,'',emission,'B');connect(emission,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat);LIB.save_loaded_asset(mat)
    REPORT['materials'][name]={'asset':mat.get_path_name(),**values,'external_textures':[]}

skeleton=require(INVENTORY['skeleton'])
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')

def options(animated):
    opt=u.FbxImportUI();opt.automated_import_should_detect_type=False
    opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=animated
    opt.import_mesh=not animated;opt.import_animations=animated;opt.create_physics_asset=False
    if animated:
        opt.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION;opt.skeleton=skeleton;data=opt.anim_sequence_import_data
        setp(data,'animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        setp(data,'use_default_sample_rate',False);setp(data,'custom_sample_rate',INVENTORY['fps']);setp(data,'remove_redundant_keys',False)
    else:
        opt.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH;data=opt.static_mesh_import_data
        setp(data,'combine_meshes',True);setp(data,'auto_generate_collision',False);setp(data,'generate_lightmap_u_vs',False)
        # Preserve authored normals, compute tangents from the explicit source UVs.
        setp(data,'normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        setp(data,'vertex_color_import_option',u.VertexColorImportOption.REPLACE)
    for name,value in [('convert_scene',True),('convert_scene_unit',True),('force_front_x_axis',False)]:setp(data,name,value)
    return opt

def import_one(name,record,animated):
    file=ROOT/record['source'];destination=ANIM_DEST if animated else MESH_DEST
    if not file.is_file():raise RuntimeError('Missing source '+record['source'])
    if LIB.does_asset_exist(destination+'/'+name):
        asset=require(destination+'/'+name);data=asset.get_editor_property('asset_import_data')
        for key,val in [('convert_scene',True),('convert_scene_unit',True),('force_front_x_axis',False)]:setp(data,key,val)
        if animated:setp(data,'use_default_sample_rate',False);setp(data,'custom_sample_rate',INVENTORY['fps'])
        LIB.save_loaded_asset(asset)
    task=u.AssetImportTask();task.filename=str(file);task.destination_path=destination;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True;task.options=options(animated);task.factory=u.FbxFactory()
    TOOLS.import_asset_tasks([task])
    typ=u.AnimSequence if animated else u.StaticMesh
    assets=[LIB.load_asset(path) for path in task.imported_object_paths]
    asset=next((a for a in assets if isinstance(a,typ)),None)
    if asset is None:raise RuntimeError('Import produced no expected asset '+name)
    expected=destination+'/'+name
    if asset.get_name()!=name:
        if LIB.does_asset_exist(expected) or not LIB.rename_asset(asset.get_path_name(),expected):raise RuntimeError('Unexpected asset naming '+asset.get_path_name())
    return asset,{'asset':asset.get_path_name(),'source':record['source'],'source_md5':hashlib.md5(file.read_bytes()).hexdigest()}

for name,record in INVENTORY['static_meshes'].items():
    mesh,result=import_one(name,record,False)
    slots=[]
    for index,slot in enumerate(mesh.static_materials):
        material_name=str(slot.material_slot_name)
        if material_name not in INVENTORY['materials']:raise RuntimeError('Unexpected material slot '+material_name)
        mesh.set_material(index,require(MATERIAL_DEST+'/'+material_name));slots.append(material_name)
    LIB.save_loaded_asset(mesh)
    bounds=mesh.get_bounding_box();lo=[bounds.min.x,bounds.min.y,bounds.min.z];hi=[bounds.max.x,bounds.max.y,bounds.max.z]
    if not all(math.isfinite(v) for v in lo+hi):raise RuntimeError('Nonfinite bounds '+name)
    source_min,source_max=record['bounds_source_cm']
    expected_min=[source_min[0],-source_max[1],source_min[2]];expected_max=[source_max[0],-source_min[1],source_max[2]]
    error=max(abs(a-b) for a,b in zip(lo+hi,expected_min+expected_max))
    if error>.15:raise RuntimeError('Import changed source bounds '+name+' error='+str(error))
    REPORT['static_meshes'][name]={**result,'bounds_ue_cm':[lo,hi],'source_bounds_error_cm':error,'material_slots':slots}

for name,record in INVENTORY['animations'].items():
    clip,result=import_one(name,record,True);setp(clip,'enable_root_motion',False);LIB.save_loaded_asset(clip)
    duration=clip.get_play_length()
    if abs(duration-record['duration'])>.006:raise RuntimeError('Action timing changed '+name+' '+str(duration))
    actual_skeleton=clip.get_editor_property('skeleton').get_path_name()
    if actual_skeleton!=skeleton.get_path_name():raise RuntimeError('Unexpected action skeleton '+name)
    REPORT['animations'][name]={**result,'duration':duration,'expected_duration':record['duration'],'skeleton':actual_skeleton}

REPORT['status']='PASS'
REPORT['counts']={'static_meshes':len(REPORT['static_meshes']),'animations':len(REPORT['animations']),'materials':len(REPORT['materials'])}
dest=ROOT/'Saved/Candidate04/WeaponAssetImport.json';dest.parent.mkdir(parents=True,exist_ok=True)
dest.write_text(json.dumps(REPORT,indent=2)+'\n',encoding='utf-8')
u.log('ONE C04 WEAPON IMPORT COMPLETE '+json.dumps(REPORT['counts']))
