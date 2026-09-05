"""Root-scheduled import of original C05 HUD atlases and assembled-weapon images."""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]; source=ROOT/'ArtSource/UI/Candidate05'
art=json.loads((source/'ui_inventory.json').read_text()); icons=json.loads((source/'weapon_icons.json').read_text())
records=[row for row in art['files'] if row['source'].endswith('.png')]+list(icons['images'].values())
assert len(records)==9
tools=u.AssetToolsHelpers.get_asset_tools(); lib=u.EditorAssetLibrary; destination='/Game/ONE/UI/Candidate05'
lib.make_directory(destination); report={'candidate':'05','status':'PENDING','textures':[]}
for record in records:
    path=(ROOT/record['source']).resolve(); assert path.is_relative_to(source.resolve())
    assert hashlib.sha256(path.read_bytes()).hexdigest()==record['sha256']
    task=u.AssetImportTask(); task.filename=str(path); task.destination_path=destination; task.destination_name=path.stem
    task.automated=True; task.replace_existing=True; task.replace_existing_settings=True; task.save=False; task.factory=u.TextureFactory()
    tools.import_asset_tasks([task]); texture=lib.load_asset(destination+'/'+path.stem); assert isinstance(texture,u.Texture2D)
    texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb',True); texture.set_editor_property('never_stream',True)
    lib.save_loaded_asset(texture)
    report['textures'].append({'asset':destination+'/'+path.stem,'source':record['source'],'source_sha256':record['sha256']})
report['status']='PASS'; output=ROOT/'Evidence/Candidate05/UIImport.json'; output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8'); u.log('CANDIDATE05_UI_IMPORT_PASS 9 original textures')
