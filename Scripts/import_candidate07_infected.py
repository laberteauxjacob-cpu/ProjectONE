"""Targeted C07 variant import; run only in the root-coordinated UE process.

Creates C07 meshes/materials/variant definition and scoped C07 physics copies.
No historical physics resave, skeleton-reference-pose update, player edit or
broad reimport.
Set PROJECTONE_C07_VARIANT to maintenance, laboratory or facility_staff.
Default maintenance preserves the established first-character command.
"""
from pathlib import Path
import hashlib, json, os
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
VARIANT_KEY=os.environ.get('PROJECTONE_C07_VARIANT','maintenance')
variants={'maintenance':'Maintenance','laboratory':'Laboratory','facility_staff':'FacilityStaff'}
assert VARIANT_KEY in variants,'Explicit known C07 variant required'
variant_id=variants[VARIANT_KEY]
manifest=json.loads((ROOT/('ArtSource/Characters/Candidate07/'+VARIANT_KEY+'_inventory.json')).read_text())
assert manifest['candidate']=='07' and manifest['variant']==VARIANT_KEY
LIB=u.EditorAssetLibrary; TOOLS=u.AssetToolsHelpers.get_asset_tools(); MEL=u.MaterialEditingLibrary
skeleton=LIB.load_asset(manifest['skeleton_asset'])
assert isinstance(skeleton,u.Skeleton), 'Existing infected skeleton is required'
assert hasattr(u,'ONEInfectedVariant'), 'Compile native C07 variant definition before importing'
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
report={'candidate':'07','variant':VARIANT_KEY,'status':'IMPORT_PASS_PENDING_RUNTIME_FIT_AND_VISUAL_REVIEW',
        'skeleton':manifest['skeleton_asset'],'meshes':{},'materials':{},'physics_scope':'C07-only copies retain C03 non-head bodies, constraints, solver and mass settings. The head shape is refitted by ONE07PhysicsAssets; authored geometry fit and runtime settling remain separate checks. Historical assets are not resaved.',
        'scope':'Source identity, explicit import settings, material bindings and variant references only. No game launch or visual quality claim.'}

def safe_asset(path):
    assert path.startswith(('/Game/ONE/Characters/Candidate07/','/Game/ONE/Materials/Candidate07/','/Game/ONE/Textures/Candidate07/'))
    return path
def source_path(relative):
    p=(ROOT/relative).resolve()
    assert p.is_relative_to((ROOT/'ArtSource/Exports/Candidate07').resolve()) and p.is_file()
    return p
textures={}
report['textures']={}
for name,d in manifest.get('textures',{}).items():
    source=(ROOT/d['source']).resolve()
    assert source.is_relative_to((ROOT/'ArtSource/Characters/Candidate07').resolve()) and source.is_file()
    assert hashlib.sha256(source.read_bytes()).hexdigest()==d['sha256']
    target=safe_asset(d['asset'])
    if VARIANT_KEY!='maintenance':
        # The two new variants reuse the existing original detail pixels without
        # reimporting or resaving Maintenance's texture assets.
        texture=LIB.load_asset(target);assert isinstance(texture,u.Texture2D)
        assert texture.get_editor_property('compression_settings')==u.TextureCompressionSettings.TC_NORMALMAP
        assert not texture.get_editor_property('srgb') and texture.get_editor_property('flip_green_channel')
        textures[name]=texture
        report['textures'][name]={'asset':target,'source':d['source'],'source_sha256':d['sha256'],'reused_without_resave':True}
        continue
    task=u.AssetImportTask();task.filename=str(source)
    task.destination_path=target.rsplit('/',1)[0];task.destination_name=name
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=False;task.factory=u.TextureFactory()
    TOOLS.import_asset_tasks([task]);texture=LIB.load_asset(target);assert isinstance(texture,u.Texture2D)
    texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
    texture.set_editor_property('srgb',False);texture.set_editor_property('flip_green_channel',True)
    LIB.save_loaded_asset(texture);textures[name]=texture
    report['textures'][name]={'asset':target,'source':d['source'],'source_sha256':d['sha256'],'normal_map':True,'flip_green_channel':True}

def material(name,d):
    path=safe_asset(d['asset'])
    if VARIANT_KEY!='maintenance':assert name.startswith('M_C07_'+variant_id+'_')
    m=LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(name,path.rsplit('/',1)[0],u.Material,u.MaterialFactoryNew())
    assert isinstance(m,u.Material)
    MEL.delete_all_material_expressions(m)
    vc=MEL.create_material_expression(m,u.MaterialExpressionVertexColor,-400,0)
    # VertexColor's aggregate is unnamed; 'RGB' silently returns False there.
    # Empty selects aggregate output 0 for both VC and TextureSample.
    assert MEL.connect_material_property(vc,'',u.MaterialProperty.MP_BASE_COLOR)
    assert MEL.connect_material_property(vc,'A',u.MaterialProperty.MP_ROUGHNESS)
    metal=MEL.create_material_expression(m,u.MaterialExpressionConstant,-400,200);metal.set_editor_property('r',d['metallic'])
    assert MEL.connect_material_property(metal,'',u.MaterialProperty.MP_METALLIC)
    if d.get('normal_texture'):
        normal=MEL.create_material_expression(m,u.MaterialExpressionTextureSample,-400,400)
        normal.set_editor_property('texture',textures[d['normal_texture']])
        normal.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        assert MEL.connect_material_property(normal,'',u.MaterialProperty.MP_NORMAL)
    # Opaque geometry: alpha controls roughness, never opacity or masking.
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
    m.set_editor_property('two_sided',False)
    MEL.set_material_usage(m,u.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    assert m.get_editor_property('used_with_skeletal_mesh'), 'C07 material must compile the skeletal mesh permutation'
    MEL.recompile_material(m);LIB.save_loaded_asset(m)
    expected=[(u.MaterialProperty.MP_BASE_COLOR,vc,''),(u.MaterialProperty.MP_ROUGHNESS,vc,'A'),(u.MaterialProperty.MP_METALLIC,metal,'')]
    if d.get('normal_texture'):expected.append((u.MaterialProperty.MP_NORMAL,normal,'RGB'))
    connections={}
    for prop,node,output in expected:
        actual_node=MEL.get_material_property_input_node(m,prop)
        actual_output=MEL.get_material_property_input_node_output_name(m,prop)
        assert actual_node==node and actual_output==output,(name,str(prop),actual_output)
        connections[str(prop)]={'node_class':actual_node.get_class().get_name(),'output':actual_output,'connected':True}
    report['materials'][name]={'asset':path,'base_color':'authored vertex RGB','roughness':'authored vertex alpha','metallic':d['metallic'],'opaque':True,'used_with_skeletal_mesh':bool(m.get_editor_property('used_with_skeletal_mesh')),'property_readback':connections}
    return m
materials={name:material(name,d) for name,d in manifest['materials'].items()}
loaded={}
for part,d in manifest['meshes'].items():
    source=source_path(d['source']);target=safe_asset(d['asset']);name=target.rsplit('/',1)[1]
    digest=hashlib.sha256(source.read_bytes()).hexdigest()
    assert digest==d['sha256'],'Source was changed after inventory; refresh reviewed source identities before import'
    options=u.FbxImportUI();options.automated_import_should_detect_type=False
    options.import_materials=False;options.import_textures=False;options.import_as_skeletal=True
    options.import_mesh=True;options.import_animations=False;options.create_physics_asset=False;options.skeleton=skeleton
    options.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH
    data=options.skeletal_mesh_import_data
    settings={'import_morph_targets':False,'use_t0_as_ref_pose':False,'update_skeleton_reference_pose':False,
              'normal_import_method':u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS,
              'vertex_color_import_option':u.VertexColorImportOption.REPLACE,
              'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False}
    for key,value in settings.items():data.set_editor_property(key,value)
    if LIB.does_asset_exist(target):
        existing=LIB.load_asset(target);old=existing.get_editor_property('asset_import_data')
        for key in settings:old.set_editor_property(key,data.get_editor_property(key))
        LIB.save_loaded_asset(existing)
    task=u.AssetImportTask();task.filename=str(source);task.destination_path=target.rsplit('/',1)[0];task.destination_name=name
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True;task.options=options;task.factory=u.FbxFactory()
    TOOLS.import_asset_tasks([task]);mesh=LIB.load_asset(target)
    assert isinstance(mesh,u.SkeletalMesh) and mesh.get_editor_property('skeleton')==skeleton
    slots=mesh.get_editor_property('materials');actual=[]
    for slot in slots:
        key=str(slot.material_slot_name);assert key in materials and key in d['materials'],key
        slot.material_interface=materials[key];actual.append(key)
    mesh.set_editor_property('materials',slots);LIB.save_loaded_asset(mesh);loaded[part]=mesh
    report['meshes'][part]={'asset':target,'source':d['source'],'source_sha256':digest,'material_slots':actual,'reference_pose_update':False}

assert hasattr(u,'ONE07PhysicsAssets'), 'Compile the scoped C07 physics authoring helper before importing'
if VARIANT_KEY=='maintenance':assert u.ONE07PhysicsAssets.build_infected_assets(), 'C07-only physics copy/refit failed'
path='/Game/ONE/Characters/Candidate07/DA_Infected_'+variant_id
if LIB.does_asset_exist(path):variant=LIB.load_asset(path)
else:
    factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.ONEInfectedVariant)
    variant=TOOLS.create_asset('DA_Infected_'+variant_id,path.rsplit('/',1)[0],u.ONEInfectedVariant,factory)
assert isinstance(variant,u.ONEInfectedVariant)
variant.set_editor_property('variant_id',variant_id);variant.set_editor_property('display_name',manifest.get('display_name','Maintenance worker'))
for part,prop in [('Core','core'),('Head','head'),('ArmLeft','arm_left'),('ArmRight','arm_right'),('LegLeft','leg_left')]:
    variant.set_editor_property(prop,loaded[part])
physics={'body_physics':'PA_Infected_C07','head_physics':'PA_Infected_Head_C07','arm_left_physics':'PA_Infected_ArmLeft_C07','arm_right_physics':'PA_Infected_ArmRight_C07','leg_left_physics':'PA_Infected_LegLeft_C07'}
for prop,name in physics.items():
    asset=LIB.load_asset(safe_asset('/Game/ONE/Characters/Candidate07/'+name));assert isinstance(asset,u.PhysicsAsset)
    variant.set_editor_property(prop,asset)
    part={'body_physics':'Core','head_physics':'Head','arm_left_physics':'ArmLeft','arm_right_physics':'ArmRight','leg_left_physics':'LegLeft'}[prop]
    loaded[part].set_editor_property('physics_asset',asset)
    assert loaded[part].get_editor_property('physics_asset')==asset
    LIB.save_loaded_asset(loaded[part])
variant.set_editor_property('voice_variation',list(variants).index(VARIANT_KEY))
# A later coordinated motion import fills the authoritative C07 map. Reimport
# never clears a map already populated by that importer.
clips=variant.get_editor_property('clips')
for key in ('Idle','Walk','Run','TurnLeft','TurnRight','SwipeLeft','SwipeRight','RakeLeft','RakeRight','TwoHand','HeavyHit','Stumble','GetUpProne','GetUpSupine'):
    clip_path='/Game/ONE/Animations/Candidate07/A_Infected_C07_'+key
    if LIB.does_asset_exist(clip_path):
        clip=LIB.load_asset(clip_path);assert isinstance(clip,u.AnimSequence) and clip.get_editor_property('skeleton')==skeleton
        clips[key]=clip
variant.set_editor_property('clips',clips);LIB.save_loaded_asset(variant)
report['definition']=path;report['clips_present']=[str(k) for k in clips]
report['physics_references']=physics
out=ROOT/('Saved/Candidate07/CharacterImport.json' if VARIANT_KEY=='maintenance' else 'Saved/Candidate07/CharacterImport_'+variant_id+'.json');out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n')
u.log('C07_'+variant_id.upper()+'_IMPORT_COMPLETE five meshes; runtime/physics/appearance review remains separate')
