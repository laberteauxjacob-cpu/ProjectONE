"""Targeted C03 audio import; root coordinates the Unreal process.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript
  -script=Scripts/import_candidate03_audio.py -AllowCommandletAudio -unattended
AllowCommandletAudio is required for the BINKA decoder. Imports exactly twelve
new sound assets. Preserves C02 sounds, clips, rigs and materials.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'ArtSource/Audio/Candidate03/manifest.json').read_text())
lib=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
destination='/Game/ONE/Audio/Weapons/Candidate03'
lib.make_directory(destination)
report={'candidate':'03','scope':'Twelve authored shot variants only; import checks do not certify perceived sound quality.','sounds':{}}
assert len(manifest['events'])==12
for name,event in manifest['events'].items():
    source=ROOT/event['source']
    assert hashlib.sha256(source.read_bytes()).hexdigest()==event['sha256'], 'Source changed '+name
    task=u.AssetImportTask()
    task.filename=str(source)
    task.destination_path=destination
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.replace_existing_settings=True
    task.save=True
    task.factory=u.SoundFactory()
    tools.import_asset_tasks([task])
    sound=lib.load_asset(event['asset'])
    assert isinstance(sound,u.SoundWave), 'Missing sound '+name
    duration=float(sound.get_editor_property('duration'))
    channels=int(sound.get_editor_property('num_channels'))
    assert abs(duration-event['duration_seconds'])<.001, 'Duration mismatch '+name
    assert channels==1, 'Channel mismatch '+name
    lib.save_loaded_asset(sound)
    report['sounds'][name]={'asset':event['asset'],'duration_seconds':duration,'channels':channels,
        'source_sha256':event['sha256'],'source_peak_dbfs':event['peak_dbfs']}
report['status']='PASS'
output=ROOT/'Evidence/Candidate03/AudioImport.json'
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n')
u.log('CANDIDATE03_AUDIO_IMPORT_PASS 12 sounds')
