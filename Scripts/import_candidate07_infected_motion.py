"""Import the 14 C07 infected-only clips and populate compatible variant maps.

Run only through the root-coordinated UE process after character import and
source metadata sanitation/inventory refresh. Existing player assets are not
loaded or reimported. Import success is not a motion/physics acceptance claim.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT/'ArtSource/Characters/Candidate07/Motion/inventory.json').read_text(encoding='utf-8'))
assert manifest['candidate'] == '07' and len(manifest['clips']) == 14
lib, tools = u.EditorAssetLibrary, u.AssetToolsHelpers.get_asset_tools()
skeleton = lib.load_asset('/Game/ONE/Characters/SK_Infected_Skeleton')
assert isinstance(skeleton, u.Skeleton), 'Existing infected skeleton required'
u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
report = {'candidate': '07', 'result': 'IMPORTED_PENDING_RUNTIME_REVIEW', 'animations': {}, 'variants': {},
          'scope': 'Source hashes, intended infected skeleton, duration and data-asset references. No evaluated motion, sound or physicality acceptance.'}
clips = {}
for name, definition in manifest['clips'].items():
    assert name == 'A_Infected_C07_' + definition['key'] and definition['skeleton'] == 'SK_Infected_Skeleton'
    source = (ROOT/definition['source']).resolve()
    assert source.is_relative_to((ROOT/'ArtSource/Exports/Candidate07/Motion').resolve()) and source.is_file()
    assert hashlib.sha256(source.read_bytes()).hexdigest() == definition['fbx_sha256'], 'Sanitized source hash must match reviewed inventory'
    options = u.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_materials = options.import_textures = False
    options.import_as_skeletal = True
    options.import_mesh = False
    options.import_animations = True
    options.create_physics_asset = False
    options.skeleton = skeleton
    options.mesh_type_to_import = u.FBXImportType.FBXIT_ANIMATION
    data = options.anim_sequence_import_data
    settings = {'animation_length': u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME,
                'use_default_sample_rate': False, 'custom_sample_rate': manifest['fps'],
                'remove_redundant_keys': False, 'convert_scene': True, 'convert_scene_unit': True, 'force_front_x_axis': False}
    for key, value in settings.items():
        data.set_editor_property(key, value)
    path = '/Game/ONE/Animations/Candidate07/' + name
    if lib.does_asset_exist(path):
        existing = lib.load_asset(path)
        assert isinstance(existing, u.AnimSequence) and existing.get_editor_property('skeleton') == skeleton
        old = existing.get_editor_property('asset_import_data')
        for key, value in settings.items():
            old.set_editor_property(key, value)
        lib.save_loaded_asset(existing)
    task = u.AssetImportTask()
    task.filename = str(source)
    task.destination_path = '/Game/ONE/Animations/Candidate07'
    task.destination_name = name
    task.automated = task.replace_existing = task.replace_existing_settings = task.save = True
    task.options = options
    task.factory = u.FbxFactory()
    tools.import_asset_tasks([task])
    clip = lib.load_asset(path)
    assert isinstance(clip, u.AnimSequence) and clip.get_editor_property('skeleton') == skeleton
    assert abs(clip.get_play_length()-definition['duration']) < .011, name
    clips[definition['key']] = clip
    lib.save_loaded_asset(clip)
    report['animations'][name] = {'asset': path, 'duration': clip.get_play_length(), 'source': definition['source'],
                                 'source_sha256': definition['fbx_sha256'], 'skeleton': skeleton.get_path_name()}
for variant_name in ('Maintenance', 'Laboratory', 'FacilityStaff'):
    path = '/Game/ONE/Characters/Candidate07/DA_Infected_' + variant_name
    if not lib.does_asset_exist(path):
        continue
    variant = lib.load_asset(path)
    assert isinstance(variant, u.ONEInfectedVariant)
    current = variant.get_editor_property('clips')
    for key, clip in clips.items():
        current[key] = clip
    variant.set_editor_property('clips', current)
    lib.save_loaded_asset(variant)
    report['variants'][variant_name] = {'asset': path, 'keys': sorted(clips)}
assert 'Maintenance' in report['variants'], 'Import the first new character definition before motion'
destination = ROOT/'Saved/Candidate07/MotionImport.json'
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
u.log('CANDIDATE07_INFECTED_MOTION_IMPORTED 14; runtime motion and audio review remain separate')
