"""Root-scheduled targeted C05 audio import. Requires -AllowCommandletAudio.
No existing sound bank is overwritten. Loop flags come from original manifest.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'ArtSource/Audio/Candidate05/manifest.json').read_text())
assert len(manifest['events'])==48
tools=u.AssetToolsHelpers.get_asset_tools(); lib=u.EditorAssetLibrary
destination='/Game/ONE/Audio/Candidate05'; lib.make_directory(destination)
report={'candidate':'05','status':'PENDING','sounds':{},'auditory_approval':False}
for name,event in manifest['events'].items():
    source=(ROOT/event['source']).resolve()
    assert source.is_relative_to((ROOT/'ArtSource/Audio/Candidate05').resolve())
    assert hashlib.sha256(source.read_bytes()).hexdigest()==event['sha256']
    task=u.AssetImportTask(); task.filename=str(source); task.destination_path=destination; task.destination_name=name
    task.automated=True; task.replace_existing=True; task.replace_existing_settings=True; task.save=False; task.factory=u.SoundFactory()
    tools.import_asset_tasks([task]); sound=lib.load_asset(destination+'/'+name); assert isinstance(sound,u.SoundWave)
    sound.set_editor_property('looping',event['loop'])
    sound.set_editor_property('volume',1.0)
    assert abs(float(sound.get_editor_property('duration'))-event['duration_seconds'])<.001
    assert int(sound.get_editor_property('num_channels'))==1
    lib.save_loaded_asset(sound)
    report['sounds'][name]={'asset':destination+'/'+name,'source_sha256':event['sha256'],'duration_seconds':event['duration_seconds'],'looping':event['loop']}
report['status']='PASS'
output=ROOT/'Evidence/Candidate05/AudioImport.json'; output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8'); u.log('CANDIDATE05_AUDIO_IMPORT_PASS 48 original sounds')
