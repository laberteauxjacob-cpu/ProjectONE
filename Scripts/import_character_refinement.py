"""Targeted last-mile garment import; no map, animation or environment mutation."""
from pathlib import Path
import unreal as u,json
ROOT=Path(__file__).resolve().parents[1]
LIB=u.EditorAssetLibrary;TOOLS=u.AssetToolsHelpers.get_asset_tools()
NAMES=('SK_Response','SK_Infected','SK_Infected_ArmL','SM_Infected_ArmL')
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
report={}
for name in NAMES:
    path='/Game/ONE/Characters/'+name
    old=LIB.load_asset(path);assert old,'Missing original imported asset '+name
    skeletal=name.startswith('SK_')
    data=old.get_editor_property('asset_import_data')
    data.set_editor_property('force_front_x_axis',False)
    data.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
    data.set_editor_property('convert_scene',True);data.set_editor_property('convert_scene_unit',True)
    if skeletal: data.set_editor_property('update_skeleton_reference_pose',False)
    LIB.save_loaded_asset(old)
    opt=u.FbxImportUI();opt.automated_import_should_detect_type=False
    opt.import_materials=False;opt.import_textures=False;opt.import_animations=False
    opt.import_mesh=True;opt.import_as_skeletal=skeletal;opt.create_physics_asset=False
    opt.mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else u.FBXImportType.FBXIT_STATIC_MESH
    if skeletal: opt.skeleton=old.skeleton;opt.skeletal_mesh_import_data=data
    else: opt.static_mesh_import_data=data
    task=u.AssetImportTask();task.filename=str(ROOT/'ArtSource'/'Exports'/(name+'.fbx'))
    task.destination_path='/Game/ONE/Characters';task.destination_name=name
    task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True
    task.options=opt;task.factory=u.FbxFactory()
    TOOLS.import_asset_tasks([task]);assert task.imported_object_paths,'Failed import '+name
    mesh=LIB.load_asset(path)
    slots=mesh.materials if skeletal else mesh.static_materials
    material_names=[]
    for i,slot in enumerate(slots):
        material_name=str(slot.material_slot_name)
        material=LIB.load_asset('/Game/ONE/Materials/'+material_name)
        assert material,'Unmapped final material '+material_name
        assert u.MaterialEditingLibrary.get_num_material_expressions(material)>=5,'Missing vertex-color material network'
        if skeletal:slot.material_interface=material
        else:mesh.set_material(i,material)
        material_names.append(material_name)
    if skeletal:
        mesh.materials=slots
        assert mesh.has_vertex_colors(),'Final mesh lacks authored vertex colors: '+name
    LIB.save_loaded_asset(mesh)
    report[name]={'path':path,'materials':material_names,'vertex_color_import':'REPLACE','force_front_x_axis':False}
(ROOT/'Evidence'/'final_garment_import.json').write_text(json.dumps(report,indent=2))
u.log('ONE FINAL GARMENT IMPORT COMPLETE '+json.dumps(report))
