"""Root-scheduled three-cue Unreal import; requires -AllowCommandletAudio."""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / 'ArtSource/Audio/Candidate06/manifest.json').read_text(encoding='utf-8'))
assert set(manifest['events']) == {'S_Pickup06_InstaKill', 'S_Pickup06_DoublePoints', 'S_Pickup06_MaxAmmo'}
tools = u.AssetToolsHelpers.get_asset_tools()
lib = u.EditorAssetLibrary
destination = '/Game/ONE/Audio/Candidate06'
lib.make_directory(destination)
report = {'candidate': '06', 'status': 'IMPORTING', 'sounds': {}, 'auditory_approval': False}
for name, event in manifest['events'].items():
    source = (ROOT / event['source']).resolve()
    assert source.is_relative_to((ROOT / 'ArtSource/Audio/Candidate06').resolve())
    assert hashlib.sha256(source.read_bytes()).hexdigest() == event['sha256']
    task = u.AssetImportTask()
    task.filename = str(source); task.destination_path = destination; task.destination_name = name
    task.automated = True; task.replace_existing = True; task.replace_existing_settings = True
    task.save = False; task.factory = u.SoundFactory()
    tools.import_asset_tasks([task])
    sound = lib.load_asset(destination + '/' + name)
    assert isinstance(sound, u.SoundWave)
    sound.set_editor_property('looping', False); sound.set_editor_property('volume', 1.)
    assert abs(float(sound.get_editor_property('duration')) - event['duration_seconds']) < .001
    assert int(sound.get_editor_property('num_channels')) == 1
    assert lib.save_loaded_asset(sound)
    report['sounds'][name] = {'asset': destination + '/' + name, 'source_sha256': event['sha256'],
        'duration_seconds': event['duration_seconds'], 'looping': False, 'recorded_source': False}
report['status'] = 'PASS'
output = ROOT / 'Evidence/Candidate06/PickupAudioImport.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
u.log('ONE06_PICKUP_AUDIO_IMPORT_PASS 3 intentional stylized collection cues')
