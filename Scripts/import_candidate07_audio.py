"""Root-scheduled import of verified C07 PCM; no claims before real import."""
from pathlib import Path
import hashlib, json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/Audio/Candidate07'
manifest=json.loads((BASE/'manifest.json').read_text(encoding='utf-8'))
assert manifest['status']=='SOURCE_PCM_VERIFIED'
tools=u.AssetToolsHelpers.get_asset_tools(); lib=u.EditorAssetLibrary
destination='/Game/ONE/Audio/Candidate07'; lib.make_directory(destination)
report={'candidate':'07','status':'IMPORTING','manifest_sha256':hashlib.sha256((BASE/'manifest.json').read_bytes()).hexdigest(),'sounds':{},'auditory_approval':False}
for name,event in manifest['events'].items():
    source=(ROOT/event['source']).resolve()
    assert source.is_relative_to((BASE/'Processed').resolve())
    assert hashlib.sha256(source.read_bytes()).hexdigest()==event['sha256']
    task=u.AssetImportTask(); task.filename=str(source); task.destination_path=destination; task.destination_name=name
    task.automated=True; task.replace_existing=True; task.replace_existing_settings=True; task.save=False; task.factory=u.SoundFactory()
    tools.import_asset_tasks([task])
    sound=lib.load_asset(destination+'/'+name); assert isinstance(sound,u.SoundWave),name
    sound.set_editor_property('looping',event['loop']); sound.set_editor_property('volume',1.)
    assert abs(float(sound.get_editor_property('duration'))-event['duration_seconds'])<.001,name
    assert int(sound.get_editor_property('num_channels'))==1,name
    assert lib.save_loaded_asset(sound),name
    report['sounds'][name]={'asset':destination+'/'+name,'source_sha256':event['sha256'],'duration_seconds':event['duration_seconds'],'looping':event['loop'],'recorded_source':True,'intentional_energy_layer':event['intentional_synthetic_energy'] is not None}
report['status']='PASS'
output=ROOT/'Saved/Candidate07/AudioImport/report.json'; output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
u.log('ONE07_AUDIO_IMPORT_PASS '+str(len(report['sounds']))+' recorded-source cues; perceptual approval not asserted')
