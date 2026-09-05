"""Targeted Candidate03 animation import. Root coordinates the UE process.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript
 -script=Scripts/import_candidate03_locomotion.py -unattended -nop4 -nullrhi
No mesh, material, bind-pose, map or accepted animation is replaced.
"""
from pathlib import Path
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT/'ArtSource/Characters/Candidate03/inventory.json').read_text())
lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
skeleton = lib.load_asset('/Game/ONE/Characters/SK_Response_Skeleton')
assert skeleton, 'Existing Response skeleton is required'
u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
report = {'skeleton': skeleton.get_path_name(), 'animations': {}, 'result': 'PASS'}
for name, definition in manifest['clips'].items():
    options = u.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_materials = False
    options.import_textures = False
    options.import_as_skeletal = True
    options.import_mesh = False
    options.import_animations = True
    options.create_physics_asset = False
    options.skeleton = skeleton
    options.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION
    data = options.anim_sequence_import_data
    data.set_editor_property('animation_length', u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    data.set_editor_property('use_default_sample_rate', False)
    data.set_editor_property('custom_sample_rate', manifest['fps'])
    data.set_editor_property('remove_redundant_keys', False)
    data.set_editor_property('convert_scene', True)
    data.set_editor_property('convert_scene_unit', True)
    data.set_editor_property('force_front_x_axis', False)
    path = '/Game/ONE/Animations/'+name
    existing = lib.load_asset(path) if lib.does_asset_exist(path) else None
    if existing:
        # AnimSequence's import-data pointer is read-only. Its settings object is
        # editable and is also consulted by the legacy FBX reimport factory.
        previous_data = existing.get_editor_property('asset_import_data')
        for property_name in ('animation_length', 'use_default_sample_rate', 'custom_sample_rate',
                              'remove_redundant_keys', 'convert_scene', 'convert_scene_unit', 'force_front_x_axis'):
            previous_data.set_editor_property(property_name, data.get_editor_property(property_name))
        lib.save_loaded_asset(existing)
    task = u.AssetImportTask()
    task.filename = str(ROOT/'ArtSource/Exports/Candidate03'/(name+'.fbx'))
    task.destination_path = '/Game/ONE/Animations'
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.options = options
    task.factory = u.FbxFactory()
    tools.import_asset_tasks([task])
    sequence = lib.load_asset(path)
    assert isinstance(sequence, u.AnimSequence), 'Missing imported sequence '+name
    assert sequence.get_editor_property('skeleton') == skeleton, 'Changed skeleton '+name
    duration = sequence.get_play_length()
    assert abs(duration-definition['duration']) < .011, 'Duration mismatch '+name
    report['animations'][name] = {'duration': duration, 'expected_duration': definition['duration'],
        'source_sample_rate': manifest['fps']}
    lib.save_loaded_asset(sequence)
destination = ROOT/'Evidence/Candidate03/LocomotionImport.json'
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, indent=2)+'\n')
u.log('CANDIDATE03_LOCOMOTION_IMPORT_PASS '+str(len(report['animations'])))
