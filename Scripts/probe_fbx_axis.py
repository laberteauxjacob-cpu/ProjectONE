"""One-mesh legacy FBX coordinate probe, used to select verified import axes."""
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[1]
u.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
o=u.FbxImportUI();o.automated_import_should_detect_type=False;o.import_materials=False;o.import_textures=False
o.import_mesh=True;o.import_as_skeletal=False;o.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
d=o.static_mesh_import_data
d.set_editor_property('force_front_x_axis',False);d.set_editor_property('convert_scene',True);d.set_editor_property('convert_scene_unit',True)
d.set_editor_property('combine_meshes',True)
t=u.AssetImportTask();t.filename=str(ROOT/'ArtSource'/'Exports'/'SM_Carbine.fbx');t.destination_path='/Game/ONE/AxisProbe';t.destination_name='SM_Carbine';t.options=o;t.factory=u.FbxFactory();t.automated=True;t.save=True;t.replace_existing=True
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
m=u.EditorAssetLibrary.load_asset(t.imported_object_paths[0])
u.log('ONE AXIS FALSE BOUNDS '+str(m.get_bounds()))
