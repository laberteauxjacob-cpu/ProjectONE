"""Targeted Candidate05 animation import. Root coordinates the UE process.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript
 -script=Scripts/import_candidate05_motion.py -unattended -nop4 -nullrhi
No mesh, material, bind-pose, map or accepted animation is replaced.
"""
from pathlib import Path
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT/'ArtSource/Characters/C05/inventory.json').read_text())
lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
skeletons = {who:lib.load_asset('/Game/ONE/Characters/SK_'+who+'_Skeleton') for who in ('Response','Infected')}
assert all(skeletons.values()), 'Both accepted skeletons are required'
u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
report = {'skeletons': {who:sk.get_path_name() for who,sk in skeletons.items()}, 'animations': {}, 'result': 'PASS', 'scope':'Targeted C05 animation import/durations/skeleton/source identity only; no rendered motion approval'}
for name, definition in manifest['clips'].items():
    skeleton=skeletons['Response' if name.startswith('A_Response_') else 'Infected']
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
    path = '/Game/ONE/Animations/Candidate05/'+name
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
    task.filename = str(ROOT/'ArtSource/Exports/Candidate05'/(name+'.fbx'))
    task.destination_path = '/Game/ONE/Animations/Candidate05'
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
        'source_sample_rate': manifest['fps'], 'asset':path, 'skeleton':skeleton.get_path_name(),
        'source':Path(task.filename).relative_to(ROOT).as_posix(), 'source_sha256':__import__('hashlib').sha256(Path(task.filename).read_bytes()).hexdigest()}
    assert report['animations'][name]['source_sha256']==definition['fbx_sha256'], 'Sanitized source/inventory mismatch '+name
    lib.save_loaded_asset(sequence)
destination = ROOT/'Saved/Candidate05/MotionImport.json'
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, indent=2)+'\n')
u.log('CANDIDATE05_MOTION_IMPORT_PASS '+str(len(report['animations'])))
