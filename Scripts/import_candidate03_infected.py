"""Targeted C03 modular infected import; root coordinates the UE process.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript
 -script=Scripts/import_candidate03_infected.py -unattended -nop4 -nullrhi
Requires the accepted infected skeleton/materials. Creates five modular meshes
and one complementary attack; no old mesh, clip, map or physics asset is replaced.
Physics assets are authored separately by root after these imports.
"""
from pathlib import Path
import hashlib,json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
MANIFEST=json.loads((ROOT/'ArtSource/Characters/Candidate03/infected_inventory.json').read_text())
LIB=u.EditorAssetLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()
skeleton=LIB.load_asset(MANIFEST['skeleton_asset'])
assert isinstance(skeleton,u.Skeleton),'Accepted infected skeleton is required'
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
report={'candidate':'03','result':'PASS','skeleton':skeleton.get_path_name(),'meshes':{},'animations':{},
    'scope':'Import verification only; physics fitting, evaluated runtime seams and gameplay contact require later checks.'}


def import_asset(name,definition,animated):
    source=ROOT/definition['source'];assert source.is_file()
    options=u.FbxImportUI();options.automated_import_should_detect_type=False
    options.import_materials=False;options.import_textures=False;options.import_as_skeletal=True
    options.import_mesh=not animated;options.import_animations=animated;options.create_physics_asset=False
    options.skeleton=skeleton
    options.mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION if animated else u.FBXImportType.FBXIT_SKELETAL_MESH
    data=options.anim_sequence_import_data if animated else options.skeletal_mesh_import_data
    if animated:
        data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate',False)
        data.set_editor_property('custom_sample_rate',MANIFEST['fps'])
        data.set_editor_property('remove_redundant_keys',False)
    else:
        data.set_editor_property('import_morph_targets',False)
        data.set_editor_property('use_t0_as_ref_pose',False)
        data.set_editor_property('update_skeleton_reference_pose',False)
        data.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        data.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
    for key,value in (('convert_scene',True),('convert_scene_unit',True),('force_front_x_axis',False)):
        data.set_editor_property(key,value)
    target=definition['asset']
    if LIB.does_asset_exist(target):
        existing=LIB.load_asset(target);old_data=existing.get_editor_property('asset_import_data')
        for key in ('convert_scene','convert_scene_unit','force_front_x_axis'):
            old_data.set_editor_property(key,data.get_editor_property(key))
        if animated:
            for key in ('use_default_sample_rate','custom_sample_rate','remove_redundant_keys'):
                old_data.set_editor_property(key,data.get_editor_property(key))
        LIB.save_loaded_asset(existing)
    task=u.AssetImportTask();task.filename=str(source);task.destination_path=target.rsplit('/',1)[0]
    task.destination_name=name;task.automated=True;task.replace_existing=True
    task.replace_existing_settings=True;task.save=True;task.options=options;task.factory=u.FbxFactory()
    TOOLS.import_asset_tasks([task])
    asset=LIB.load_asset(target)
    assert isinstance(asset,u.AnimSequence if animated else u.SkeletalMesh),'Import failed: '+name
    assert asset.get_editor_property('skeleton')==skeleton,'Unexpected skeleton: '+name
    result={'asset':asset.get_path_name(),'source':definition['source'],
        'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
    if animated:
        length=asset.get_play_length();assert abs(length-definition['duration'])<.011
        result.update(duration_seconds=length,contact_seconds=definition['contact_seconds'])
    else:
        slots=asset.get_editor_property('materials');names=[]
        for slot in slots:
            slot_name=str(slot.material_slot_name)
            material=LIB.load_asset('/Game/ONE/Materials/'+slot_name)
            assert isinstance(material,u.MaterialInterface),'Missing accepted material '+slot_name
            slot.material_interface=material;names.append(slot_name)
        asset.set_editor_property('materials',slots)
        # Legacy import may omit unused source slots inherited from the joined
        # accepted body. Every retained slot still maps to its original material.
        assert set(names).issubset(set(definition['materials'])),'Unexpected material slot '+name
        result.update(material_slots=names,source_vertices=definition['vertices'],source_triangles=definition['triangles'])
    LIB.save_loaded_asset(asset)
    return result


for name,definition in MANIFEST['meshes'].items():report['meshes'][name]=import_asset(name,definition,False)
for name,definition in MANIFEST['clips'].items():report['animations'][name]=import_asset(name,definition,True)
output=ROOT/'Evidence/Candidate03/StageD/InfectedImport.json';output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n')
u.log('C03_INFECTED_IMPORT_PASS five meshes / one complementary attack; existing skeleton reused')
