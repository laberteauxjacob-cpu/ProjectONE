"""Read-only Candidate05 asset/catalog verification; run in the root's UE window.

UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript
 -script=Scripts/verify_candidate05_assets.py -unattended -nop4 -nullrhi
 -AllowCommandletAudio

Loads assets and the native weapon component CDO but never imports, rewrites
metadata, saves packages, starts gameplay or changes catalog properties.
Writes only Saved/Candidate05/AssetVerification.json and fails the commandlet
if any required reference, source identity or measured property differs.
"""
from pathlib import Path
import hashlib
import json
import math
import re
import struct
import wave
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
OUTPUT=ROOT/'Saved/Candidate05/AssetVerification.json'
REPORT={'candidate':'05','status':'RUNNING','read_only_assets':True,'animations':[],
        'textures':[],'audio':[],'catalog':[],'materials':[],'failures':[],
        'limitations':'Asset identity/reference/property checks only; no visual approval, audible mix claim or gameplay transaction test.'}

class Mismatch(RuntimeError): pass

def require(condition,code):
    if not condition: raise Mismatch(code)

def prop(obj,native_name):
    # The reflected native names are stable; Python aliases strip boolean b.
    try: return obj.get_editor_property(native_name)
    except Exception:
        name=native_name[1:] if len(native_name)>1 and native_name[0]=='b' and native_name[1].isupper() else native_name
        alias=re.sub(r'(?<!^)(?=[A-Z])','_',name).lower()
        try: return obj.get_editor_property(alias)
        except Exception: raise Mismatch('reflected_property_unavailable_'+native_name) from None

def source_path(relative):
    require(isinstance(relative,str) and not re.match(r'^(?:[A-Za-z]:|[/\\])',relative),'source_path_must_be_relative')
    path=(ROOT/relative).resolve()
    require(path.is_relative_to((ROOT/'ArtSource').resolve()) and path.is_file(),'source_missing_or_outside_ArtSource')
    return path

def digest(path,algorithm='sha256'):
    return hashlib.new(algorithm,path.read_bytes()).hexdigest()

def load(path,kind):
    require(path.startswith('/Game/ONE/'),'asset_outside_project')
    result=u.EditorAssetLibrary.load_asset(path)
    require(isinstance(result,kind),'missing_or_wrong_asset_type')
    return result

def serialized_import_info(data):
    # UE stores FAssetImportInfo twice here: its asset-registry tag and the
    # AssetImportData export. Both are length-prefixed FStrings and both must
    # agree. Inspect every copy, rather than trusting the first regex match.
    key='"RelativeFilename"'
    patterns=[(rb'\[\s*\{\s*"RelativeFilename"',1,'utf-8'),
              (rb'\[\x00(?:[ \t\r\n]\x00)*\{\x00(?:[ \t\r\n]\x00)*'+re.escape(key.encode('utf-16-le')),2,'utf-16-le')]
    copies=[]; encodings=[]
    for pattern,unit,encoding in patterns:
        for match in re.finditer(pattern,data):
            start=match.start(); require(start>=4,'import_info_missing_FString_length')
            count=struct.unpack_from('<i',data,start-4)[0]
            require((count>0 if unit==1 else count<0),'import_info_FString_encoding_mismatch')
            size=abs(count)*unit; require(unit<size<1048576 and start+size<=len(data),'import_info_FString_length_invalid')
            payload=data[start:start+size]
            require(payload[-unit:]==b'\0'*unit,'import_info_FString_not_terminated')
            info=json.loads(payload[:-unit].decode(encoding))
            require(isinstance(info,list) and len(info)==1 and isinstance(info[0],dict),'expected_one_source_record')
            require({'RelativeFilename','FileMD5'}.issubset(info[0]),'import_info_source_fields_missing')
            copies.append(info); encodings.append(encoding)
    require(bool(copies),'serialized_import_info_missing')
    require(all(copy==copies[0] for copy in copies),'serialized_import_info_copies_disagree')
    return copies[0][0],len(copies),sorted(set(encodings))

def imported_identity(asset,relative,expected_sha):
    source=source_path(relative); sha=digest(source)
    require(sha==expected_sha,'source_sha256_differs_from_authored_inventory')
    package_path=asset.get_path_name().split('.')[0]
    require(package_path.startswith('/Game/ONE/'),'invalid_imported_package_path')
    package=ROOT/'Content'/(package_path[len('/Game/'):]+'.uasset')
    require(package.is_file(),'serialized_package_missing')
    record,copies,encodings=serialized_import_info(package.read_bytes())
    recorded=str(record['RelativeFilename'])
    require(not re.match(r'^(?:[A-Za-z]:|[/\\])',recorded),'absolute_import_source_path')
    require(not re.search(r'(?i)(?:[A-Za-z]:[\\/]|/(?:Users|home)/)',recorded),'private_import_source_path')
    require((package.parent/recorded).resolve()==source,'portable_import_path_resolves_to_wrong_source')
    md5=digest(source,'md5'); require(str(record['FileMD5']).lower()==md5,'serialized_source_MD5_mismatch')
    data=prop(asset,'AssetImportData')
    names=list(data.extract_filenames()); require(len(names)==1,'loaded_import_source_count_mismatch')
    require(Path(names[0]).resolve()==source,'loaded_import_source_resolves_to_wrong_source')
    return {'asset':package_path,'source':relative,'source_sha256':sha,'source_md5':md5,'portable_import_path':recorded,
            'matching_serialized_import_copies':copies,'import_info_encodings':encodings}

def check(section,key,callback):
    try:
        result=callback(); REPORT[section].append(result)
    except Exception as error:
        # Exception strings from engine APIs can include private paths. Only
        # deliberate portable mismatch codes are copied into this report.
        REPORT['failures'].append({'section':section,'item':key,
            'reason':str(error) if isinstance(error,Mismatch) else type(error).__name__})
        u.log_error('CANDIDATE05_ASSET_CHECK_FAILED '+section+' '+key+' '+type(error).__name__)

def animation(name,definition):
    path='/Game/ONE/Animations/Candidate05/'+name
    asset=load(path,u.AnimSequence)
    row=imported_identity(asset,'ArtSource/Exports/Candidate05/'+name+'.fbx',definition['fbx_sha256'])
    expected='/Game/ONE/Characters/'+definition['skeleton']
    skeleton=prop(asset,'Skeleton'); require(skeleton is not None,'missing_animation_skeleton')
    require(skeleton.get_path_name().split('.')[0]==expected,'animation_skeleton_mismatch')
    duration=float(asset.get_play_length()); require(math.isfinite(duration) and abs(duration-definition['duration'])<.011,'animation_duration_mismatch')
    row.update(duration_seconds=duration,expected_duration_seconds=definition['duration'],skeleton=expected)
    return row

def texture(record):
    source=source_path(record['source']); path='/Game/ONE/UI/Candidate05/'+source.stem
    asset=load(path,u.Texture2D); row=imported_identity(asset,record['source'],record['sha256'])
    data=source.read_bytes(); require(data[:8]==b'\x89PNG\r\n\x1a\n','texture_source_not_PNG')
    width,height=struct.unpack('>II',data[16:24])
    measured=[int(asset.blueprint_get_size_x()),int(asset.blueprint_get_size_y())]
    require(measured==[width,height],'imported_texture_dimensions_mismatch')
    require(prop(asset,'CompressionSettings')==u.TextureCompressionSettings.TC_EDITOR_ICON,'texture_compression_not_UI')
    require(prop(asset,'LODGroup')==u.TextureGroup.TEXTUREGROUP_UI,'texture_LOD_group_not_UI')
    require(prop(asset,'MipGenSettings')==u.TextureMipGenSettings.TMGS_NO_MIPMAPS,'unexpected_UI_mipmaps')
    require(bool(prop(asset,'NeverStream')),'UI_texture_can_stream')
    row.update(dimensions=measured); return row

def sound(name,event):
    asset=load(event['asset'],u.SoundWave); row=imported_identity(asset,event['source'],event['sha256'])
    source=source_path(event['source'])
    with wave.open(str(source),'rb') as wav:
        frames=wav.getnframes(); rate=wav.getframerate(); duration=frames/rate
        require((wav.getnchannels(),rate,wav.getsampwidth())==(1,48000,2),'audio_source_format_mismatch')
    measured=float(prop(asset,'Duration')); looping=bool(prop(asset,'Looping'))
    require(math.isfinite(measured) and abs(measured-duration)<.001 and abs(duration-event['duration_seconds'])<1/48000,'audio_duration_mismatch')
    require(looping==bool(event['loop']),'audio_loop_flag_mismatch')
    require(int(prop(asset,'NumChannels'))==1,'imported_audio_channels_mismatch')
    row.update(duration_seconds=measured,looping=looping,source_samples=frames); return row

def referenced(value,kind,required=True):
    if value is None:
        require(not required,'required_catalog_reference_empty'); return None
    if isinstance(value,u.Object): asset=value
    else:
        # SoftObjectPath wrappers and text exports both carry the real reflected
        # /Game object path. Never reconstruct a missing name from the manifest.
        text=str(value); paths=re.findall(r'/Game/ONE/[A-Za-z0-9_/]+(?:\.[A-Za-z0-9_]+)?',text)
        if not paths:
            require(not required and text in ('None',''), 'unreadable_catalog_soft_reference'); return None
        require(len(set(paths))==1,'ambiguous_catalog_soft_reference'); asset=u.EditorAssetLibrary.load_asset(paths[0])
    require(isinstance(asset,kind),'catalog_reference_missing_or_wrong_type')
    result=asset.get_path_name().split('.')[0]; require(result.startswith('/Game/ONE/'),'catalog_reference_outside_project')
    if isinstance(asset,u.AnimSequence):
        sk=prop(asset,'Skeleton'); require(sk is not None and sk.get_name()=='SK_Response_Skeleton','catalog_clip_wrong_skeleton')
        require(float(asset.get_play_length())>0,'catalog_clip_has_no_duration')
    return result

def catalog_row(definition):
    identity=str(prop(definition,'Id')); upgraded=bool(prop(definition,'bUpgraded'))
    family=prop(definition,'Family'); name=str(prop(definition,'DisplayName'))
    expected={'AR01':('M4A1',u.ONEWeaponFamily.CARBINE,False),'SG01':('Remington 870',u.ONEWeaponFamily.SHOTGUN,False),
              'P1911':('M1911',u.ONEWeaponFamily.PISTOL,False),'AR01_UP':('Overcurrent',u.ONEWeaponFamily.CARBINE,True),
              'SG01_UP':('Gravebreaker',u.ONEWeaponFamily.SHOTGUN,True),'P1911_UP':('Last Word',u.ONEWeaponFamily.PISTOL,True)}
    require(identity in expected and (name,family,upgraded)==expected[identity],'catalog_identity_mismatch')
    refs={}
    for field in ('Mesh','EjectedCaseMesh','MagazineMesh','ForeEndMesh','SlideMesh','ShellMesh'):
        required=field in ('Mesh','EjectedCaseMesh') or (field=='MagazineMesh' and family!=u.ONEWeaponFamily.SHOTGUN) or \
            (field in ('ForeEndMesh','ShellMesh') and family==u.ONEWeaponFamily.SHOTGUN) or (field=='SlideMesh' and family==u.ONEWeaponFamily.PISTOL)
        value=prop(definition,field)
        if value is not None or required: refs[field]=referenced(value,u.StaticMesh,required)
    refs['ReadyAnimation']=referenced(prop(definition,'ReadyAnimation'),u.AnimSequence)
    refs['EmptySound']=referenced(prop(definition,'EmptySound'),u.SoundBase)
    for field,count in (('ShotSounds',6),('FleshSounds',3),('ConcreteSounds',2),('MetalSounds',2)):
        values=list(prop(definition,field)); require(len(values)==count,'catalog_audio_bank_count_mismatch')
        refs[field]=[referenced(value,u.SoundBase) for value in values]
        require(len(set(refs[field]))==count,'catalog_audio_variants_duplicate')
    if upgraded:
        stem={'AR01_UP':'Overcurrent','SG01_UP':'Gravebreaker','P1911_UP':'LastWord'}[identity]
        require(refs['ShotSounds']==['/Game/ONE/Audio/Candidate05/S_'+stem+'Shot_'+str(i).zfill(2) for i in range(1,7)],'catalog_upgraded_bank_not_current_C05')
    operations=[]
    for operation in prop(definition,'Operations'):
        row={'operation':str(prop(operation,'Operation')),'animation':referenced(prop(operation,'Animation'),u.AnimSequence),'event_sounds':[]}
        for event in prop(operation,'Events'):
            value=prop(event,'Sound')
            if value is not None: row['event_sounds'].append(referenced(value,u.SoundBase,False))
        operations.append(row)
    require(len(operations)==(6 if family==u.ONEWeaponFamily.SHOTGUN else 3),'catalog_operation_count_mismatch')
    return {'id':identity,'name':name,'upgraded':upgraded,'references':refs,'operations':operations}

def main():
    motion=json.loads((ROOT/'ArtSource/Characters/C05/inventory.json').read_text())
    ui=json.loads((ROOT/'ArtSource/UI/Candidate05/ui_inventory.json').read_text())
    icons=json.loads((ROOT/'ArtSource/UI/Candidate05/weapon_icons.json').read_text())
    audio=json.loads((ROOT/'ArtSource/Audio/Candidate05/manifest.json').read_text())
    records=[row for row in ui['files'] if row['source'].endswith('.png')]+list(icons['images'].values())
    require(len(motion['clips'])==25 and len(records)==9 and len(audio['events'])==48,'authored_inventory_count_mismatch')
    require(len({x['source'] for x in records})==9,'duplicate_UI_source')
    for name,definition in motion['clips'].items(): check('animations',name,lambda n=name,d=definition:animation(n,d))
    for record in records: check('textures',Path(record['source']).stem,lambda r=record:texture(r))
    for name,event in audio['events'].items(): check('audio',name,lambda n=name,e=event:sound(n,e))
    def read_catalog():
        native=u.load_class(None,'/Script/ProjectONE.ONEWeaponComponent'); require(native is not None,'native_weapon_component_class_missing')
        cdo=u.get_default_object(native); definitions=list(prop(cdo,'WeaponDefinitions'))
        require(len(definitions)==6,'native_catalog_count_mismatch')
        for index,definition in enumerate(definitions): check('catalog','row_'+str(index),lambda d=definition:catalog_row(d))
    try: read_catalog()
    except Exception as error:
        REPORT['failures'].append({'section':'catalog','item':'native_CDO','reason':str(error) if isinstance(error,Mismatch) else type(error).__name__})
    check('materials','M_Tracer_C04',lambda:{'asset':load('/Game/ONE/Materials/M_Tracer_C04',u.MaterialInterface).get_path_name().split('.')[0],
        'purpose':'Existing runtime tracer / held upgraded aura material'})
    require(len(REPORT['catalog'])==6 or REPORT['failures'],'catalog_not_checked')

try:
    main()
except Exception as error:
    REPORT['failures'].append({'section':'setup','item':'inventories','reason':str(error) if isinstance(error,Mismatch) else type(error).__name__})
finally:
    REPORT['verified_counts']={key:len(REPORT[key]) for key in ('animations','textures','audio','catalog','materials')}
    expected={'animations':25,'textures':9,'audio':48,'catalog':6,'materials':1}
    REPORT['status']='PASS' if not REPORT['failures'] and REPORT['verified_counts']==expected else 'FAIL'
    OUTPUT.parent.mkdir(parents=True,exist_ok=True); OUTPUT.write_text(json.dumps(REPORT,indent=2)+'\n',encoding='utf-8')
if REPORT['status']!='PASS': raise RuntimeError('CANDIDATE05_ASSET_VERIFICATION_FAILED; see Saved/Candidate05/AssetVerification.json')
u.log('CANDIDATE05_ASSET_VERIFICATION_PASS 25 animations / 9 textures / 48 sounds / 6 native catalog rows / aura material')
