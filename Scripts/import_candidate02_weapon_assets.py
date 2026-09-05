"""Targeted Candidate02 import. Run through UnrealEditor-Cmd -run=pythonscript.

Requires the accepted Candidate01 assets already present. Imports only the
explicit inventory below; never rebuilds C01 meshes, materials or animations.
Gameplay event timing lives in editable native weapon definitions.
"""
from pathlib import Path
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
INVENTORY = json.loads((ROOT/'ArtSource/Weapons/Candidate02/inventory.json').read_text())
TOOLS = u.AssetToolsHelpers.get_asset_tools()
LIB = u.EditorAssetLibrary
MEL = u.MaterialEditingLibrary
REPORT = {'candidate': '02', 'static_meshes': {}, 'animations': {}, 'audio': {}}


def setp(obj, name, value):
    obj.set_editor_property(name, value)


def required(path):
    asset = LIB.load_asset(path)
    if asset is None:
        raise RuntimeError('Required existing asset missing: '+path)
    return asset


def import_file(file, destination, options=None, factory=None):
    if not file.is_file():
        raise RuntimeError('Missing authored source: '+str(file))
    target = destination+'/'+file.stem
    existing = LIB.load_asset(target) if LIB.does_asset_exist(target) else None
    # Legacy FBX reimport consults saved flags before the task options.
    if existing and file.suffix == '.fbx':
        data = existing.get_editor_property('asset_import_data')
        setp(data, 'force_front_x_axis', False)
        setp(data, 'convert_scene', True)
        setp(data, 'convert_scene_unit', True)
        if isinstance(existing, u.AnimSequence):
            setp(data, 'use_default_sample_rate', False)
            setp(data, 'custom_sample_rate', INVENTORY['fps'])
        LIB.save_loaded_asset(existing)
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = destination
    task.destination_name = file.stem
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    if options:
        task.options = options
    if factory:
        task.factory = factory
    TOOLS.import_asset_tasks([task])
    objects = [LIB.load_asset(path) for path in task.imported_object_paths]
    objects = [asset for asset in objects if asset]
    if not objects:
        raise RuntimeError('Import produced no objects: '+file.name)
    u.log('ONE C02 IMPORT: '+file.name+' -> '+str(task.imported_object_paths))
    return objects


def options_for(skeleton=None):
    options = u.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_materials = False
    options.import_textures = False
    options.import_as_skeletal = skeleton is not None
    options.import_mesh = skeleton is None
    options.import_animations = skeleton is not None
    options.create_physics_asset = False
    if skeleton:
        options.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION
        options.skeleton = skeleton
        data = options.anim_sequence_import_data
        setp(data, 'animation_length', u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        setp(data, 'use_default_sample_rate', False)
        setp(data, 'custom_sample_rate', INVENTORY['fps'])
        setp(data, 'remove_redundant_keys', False)
    else:
        options.mesh_type_to_import = u.FBXImportType.FBXIT_STATIC_MESH
        data = options.static_mesh_import_data
        setp(data, 'combine_meshes', True)
        setp(data, 'auto_generate_collision', False)
        setp(data, 'generate_lightmap_u_vs', False)
        setp(data, 'normal_import_method', u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        setp(data, 'vertex_color_import_option', u.VertexColorImportOption.REPLACE)
    setp(data, 'convert_scene', True)
    setp(data, 'convert_scene_unit', True)
    setp(data, 'force_front_x_axis', False)
    return options


u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
for destination in ('Art/Weapons', 'Audio/Weapons', 'Animations'):
    LIB.make_directory('/Game/ONE/'+destination)
skeletons = {who: required('/Game/ONE/Characters/SK_'+who).skeleton for who in ('Response', 'Infected')}

# Three new material definitions only. Existing C01 material graphs are reused.
for name, (color, roughness, metallic) in INVENTORY['materials'].items():
    destination = '/Game/ONE/Materials'
    material = LIB.load_asset(destination+'/'+name) if LIB.does_asset_exist(destination+'/'+name) else TOOLS.create_asset(name, destination, u.Material, u.MaterialFactoryNew())
    MEL.delete_all_material_expressions(material)
    base = MEL.create_material_expression(material, u.MaterialExpressionConstant3Vector, -300, 0)
    setp(base, 'constant', u.LinearColor(*color))
    MEL.connect_material_property(base, '', u.MaterialProperty.MP_BASE_COLOR)
    for value, prop, y in ((roughness, u.MaterialProperty.MP_ROUGHNESS, 180), (metallic, u.MaterialProperty.MP_METALLIC, 290)):
        node = MEL.create_material_expression(material, u.MaterialExpressionConstant, -300, y)
        setp(node, 'r', value)
        MEL.connect_material_property(node, '', prop)
    MEL.recompile_material(material)
    LIB.save_loaded_asset(material)

for name in INVENTORY['static_meshes']:
    assets = import_file(ROOT/'ArtSource/Exports'/(name+'.fbx'), '/Game/ONE/Art/Weapons', options_for(), u.FbxFactory())
    mesh = next(asset for asset in assets if isinstance(asset, u.StaticMesh))
    slots = []
    for index, slot in enumerate(mesh.static_materials):
        material_name = str(slot.material_slot_name)
        mesh.set_material(index, required('/Game/ONE/Materials/'+material_name))
        slots.append(material_name)
    LIB.save_loaded_asset(mesh)
    bounds = mesh.get_bounding_box()
    REPORT['static_meshes'][name] = {'asset': mesh.get_path_name(), 'material_slots': slots,
        'bounds_min': [bounds.min.x, bounds.min.y, bounds.min.z], 'bounds_max': [bounds.max.x, bounds.max.y, bounds.max.z]}

animations = dict(INVENTORY['animations'])
reaction = INVENTORY['infected_reaction']
animations[reaction['name']] = reaction
for name, definition in animations.items():
    who = 'Response' if name.startswith('A_Response_') else 'Infected'
    assets = import_file(ROOT/'ArtSource/Exports'/(name+'.fbx'), '/Game/ONE/Animations', options_for(skeletons[who]), u.FbxFactory())
    sequence = next(asset for asset in assets if isinstance(asset, u.AnimSequence))
    target = '/Game/ONE/Animations/'+name
    if sequence.get_name() != name:
        if LIB.does_asset_exist(target):
            raise RuntimeError('Unexpected duplicate animation import: '+sequence.get_path_name())
        if not LIB.rename_asset(sequence.get_path_name(), target):
            raise RuntimeError('Unable to assign animation name: '+name)
    setp(sequence, 'enable_root_motion', False)
    LIB.save_loaded_asset(sequence)
    duration = sequence.get_play_length()
    if abs(duration-definition['duration']) > .006:
        raise RuntimeError('Authored timing changed at import: '+name+' '+str(duration))
    REPORT['animations'][name] = {'asset': sequence.get_path_name(), 'duration': duration,
        'expected_duration': definition['duration'], 'skeleton': sequence.skeleton.get_path_name()}

for name, definition in INVENTORY['audio'].items():
    assets = import_file(ROOT/definition['source'], '/Game/ONE/Audio/Weapons', factory=u.SoundFactory())
    sound = next(asset for asset in assets if isinstance(asset, u.SoundWave))
    # Authored event peaks already leave headroom; runtime spatial gain remains editable.
    LIB.save_loaded_asset(sound)
    REPORT['audio'][name] = {'asset': sound.get_path_name(), 'source_duration': definition['duration'],
        'source_peak_dbfs': definition['peak_dbfs'], 'source_rms_dbfs': definition['rms_dbfs']}

REPORT['status'] = 'PASS'
report_path = ROOT/'Saved/Candidate02/WeaponAssetImport.json'
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text(json.dumps(REPORT, indent=2))
u.log('ONE C02 IMPORT COMPLETE: 5 meshes, 9 animations, 25 sounds; report '+str(report_path))
