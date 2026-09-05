"""Targeted Candidate04 original audio import; root alone schedules Unreal.
Requires -AllowCommandletAudio for BINKA encoding. Imports exactly28 new assets.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'ArtSource/Audio/Candidate04/manifest.json').read_text())
assert len(manifest['events'])==28
tools=u.AssetToolsHelpers.get_asset_tools()
lib=u.EditorAssetLibrary
destination='/Game/ONE/Audio/Weapons/Candidate04'
lib.make_directory(destination)
report={'candidate':'04','status':'PENDING','scope':'28 original weapon audio assets; import verification is not perceptual approval.','sounds':{}}
for name,event in manifest['events'].items():
    source=ROOT/event['source']
    assert source.resolve().is_relative_to((ROOT/'ArtSource/Audio/Candidate04').resolve())
    assert hashlib.sha256(source.read_bytes()).hexdigest()==event['sha256'], 'Source changed '+name
    assert event['asset']==destination+'/'+name
    task=u.AssetImportTask()
    task.filename=str(source); task.destination_path=destination; task.destination_name=name
    task.automated=True; task.replace_existing=True; task.replace_existing_settings=True; task.save=True
    task.factory=u.SoundFactory()
    tools.import_asset_tasks([task])
    sound=lib.load_asset(event['asset'])
    assert isinstance(sound,u.SoundWave), 'Missing sound '+name
    duration=float(sound.get_editor_property('duration')); channels=int(sound.get_editor_property('num_channels'))
    assert abs(duration-event['duration_seconds'])<.001 and channels==1, 'Format mismatch '+name
    lib.save_loaded_asset(sound)
    report['sounds'][name]={'asset':event['asset'],'source':event['source'],'source_sha256':event['sha256'],
                           'duration_seconds':duration,'channels':channels,'source_peak_dbfs':event['peak_dbfs']}
report['status']='PASS'
output=ROOT/'Evidence/Candidate04/AudioImport.json'; output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n')
u.log('CANDIDATE04_AUDIO_IMPORT_PASS 28 sounds')
