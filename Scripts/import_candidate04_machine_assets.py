"""Targeted import of original C04 machine meshes, materials and sound cues.

Run through the coordinated UnrealEditor-Cmd Python gate with
-AllowCommandletAudio (required by the installed BINKA decoder). This importer
does not load or alter the arena, navigation, older assets or runtime actors.
Machine meshes are visual only; the owning placed actor supplies its explicitly
sized blocking/nav component, so no implicit or accumulating FBX hull is made.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/Machines/Candidate04'
inventory=json.loads((SOURCE/'inventory.json').read_text())
audio=json.loads((SOURCE/'Audio/manifest.json').read_text())
lib=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools();mel=u.MaterialEditingLibrary
MATERIALS='/Game/ONE/Materials/Machines/Candidate04'
report={'candidate':'04','scope':'New original machine presentation only','materials':{},'meshes':{},'audio':{}}

def connect(ok,label):
    if not ok:raise RuntimeError('Rejected material graph connection: '+label)

lib.make_directory(MATERIALS)
for name,row in inventory['materials'].items():
    asset=MATERIALS+'/'+name
    m=lib.load_asset(asset) if lib.does_asset_exist(asset) else tools.create_asset(name,MATERIALS,u.Material,u.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
    color=mel.create_material_expression(m,u.MaterialExpressionConstant3Vector,-500,0)
    color.set_editor_property('constant',u.LinearColor(*row['color']))
    vertex=mel.create_material_expression(m,u.MaterialExpressionVertexColor,-500,160)
    multiply=mel.create_material_expression(m,u.MaterialExpressionMultiply,-230,0)
    connect(mel.connect_material_expressions(color,'',multiply,'A'),name+' color')
    connect(mel.connect_material_expressions(vertex,'',multiply,'B'),name+' vertex color')
    connect(mel.connect_material_property(multiply,'',u.MaterialProperty.MP_BASE_COLOR),name+' base')
    for key,prop,y in [('roughness',u.MaterialProperty.MP_ROUGHNESS,310),('metallic',u.MaterialProperty.MP_METALLIC,430)]:
        value=mel.create_material_expression(m,u.MaterialExpressionConstant,-230,y);value.set_editor_property('r',row[key])
        connect(mel.connect_material_property(value,'',prop),name+' '+key)
    if row.get('emission'):
        glow=mel.create_material_expression(m,u.MaterialExpressionVectorParameter,-520,620)
        glow.set_editor_property('parameter_name','GlowColor');glow.set_editor_property('default_value',u.LinearColor(*row['color']))
        gain=mel.create_material_expression(m,u.MaterialExpressionScalarParameter,-520,760)
        gain.set_editor_property('parameter_name','GlowStrength');gain.set_editor_property('default_value',row['emission'])
        gm=mel.create_material_expression(m,u.MaterialExpressionMultiply,-230,620)
        connect(mel.connect_material_expressions(glow,'',gm,'A'),name+' emission color')
        connect(mel.connect_material_expressions(gain,'',gm,'B'),name+' emission strength')
        connect(mel.connect_material_property(gm,'',u.MaterialProperty.MP_EMISSIVE_COLOR),name+' emissive output')
    mel.recompile_material(m)
    if mel.get_material_property_input_node(m,u.MaterialProperty.MP_BASE_COLOR)!=multiply:
        raise RuntimeError('Material base graph readback mismatch '+name)
    if not lib.save_loaded_asset(m):raise RuntimeError('Cannot save material '+name)
    report['materials'][name]={'asset':asset,'vertex_color':True,'parameterized_emission':bool(row.get('emission'))}

u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
for name,row in inventory['static_meshes'].items():
    source=ROOT/row['source'];digest=hashlib.sha256(source.read_bytes()).hexdigest()
    if digest!=row['source_sha256']:raise RuntimeError('Source hash differs from machine inventory: '+name)
    options=u.FbxImportUI();options.automated_import_should_detect_type=False
    options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False
    options.import_mesh=True;options.import_animations=False;options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
    data=options.static_mesh_import_data
    for key,value in {'combine_meshes':True,'auto_generate_collision':False,'generate_lightmap_u_vs':False,
        'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,
        'normal_import_method':u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS,
        'vertex_color_import_option':u.VertexColorImportOption.REPLACE}.items():data.set_editor_property(key,value)
    if lib.does_asset_exist(row['asset']):
        existing=lib.load_asset(row['asset']);saved=existing.get_editor_property('asset_import_data')
        for key in ('convert_scene','convert_scene_unit','force_front_x_axis','auto_generate_collision'):
            saved.set_editor_property(key,data.get_editor_property(key))
    task=u.AssetImportTask();task.filename=str(source);task.destination_path=row['asset'].rsplit('/',1)[0]
    task.destination_name=name;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True
    task.save=True;task.options=options;task.factory=u.FbxFactory();tools.import_asset_tasks([task])
    mesh=lib.load_asset(row['asset'])
    if not isinstance(mesh,u.StaticMesh):raise RuntimeError('No imported mesh '+name)
    for i,slot in enumerate(mesh.static_materials):
        material=lib.load_asset(MATERIALS+'/'+str(slot.material_slot_name))
        if not material:raise RuntimeError('Unknown machine material '+str(slot.material_slot_name))
        mesh.set_material(i,material)
    bounds=mesh.get_bounding_box();actual={'min':[bounds.min.x,bounds.min.y,bounds.min.z],'max':[bounds.max.x,bounds.max.y,bounds.max.z]}
    max_error=max(abs(a-b) for side in ('min','max') for a,b in zip(actual[side],row['ue_bounds_local_cm'][side]))
    if max_error>.03:raise RuntimeError('Machine FBX axis/pivot/scale mismatch '+name+': '+str(actual))
    if not lib.save_loaded_asset(mesh):raise RuntimeError('Cannot save machine '+name)
    report['meshes'][name]={'asset':row['asset'],'source':row['source'],'source_sha256':digest,'bounds_cm':actual,'max_bounds_error_cm':max_error,'import_auto_collision':False}

for name,row in audio['events'].items():
    source=ROOT/row['source'];digest=hashlib.sha256(source.read_bytes()).hexdigest()
    if digest!=row['sha256']:raise RuntimeError('Machine audio source hash changed '+name)
    task=u.AssetImportTask();task.filename=str(source);task.destination_path=row['asset'].rsplit('/',1)[0]
    task.destination_name=name;task.automated=True;task.replace_existing=True;task.replace_existing_settings=True
    task.save=True;task.factory=u.SoundFactory();tools.import_asset_tasks([task])
    sound=lib.load_asset(row['asset'])
    if not isinstance(sound,u.SoundWave):raise RuntimeError('No machine sound '+name)
    sound.set_editor_property('looping',row['looping'])
    duration=float(sound.get_editor_property('duration'));channels=int(sound.get_editor_property('num_channels'))
    if abs(duration-row['duration_seconds'])>.001 or channels!=1:raise RuntimeError('Audio format/duration mismatch '+name)
    if not lib.save_loaded_asset(sound):raise RuntimeError('Cannot save machine sound '+name)
    report['audio'][name]={'asset':row['asset'],'source':row['source'],'source_sha256':digest,'seconds':duration,'looping':row['looping']}

report['result']='PASS';out=ROOT/'Saved/Candidate04/MachineImport.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n')
u.log('CANDIDATE04_MACHINE_IMPORT_PASS '+str(len(report['meshes']))+' meshes, '+str(len(report['materials']))+' materials, '+str(len(report['audio']))+' sounds')
