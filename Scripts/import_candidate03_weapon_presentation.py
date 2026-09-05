"""Targeted Stage C import: one original brass mesh and one flash material.

Run only through the coordinated UnrealEditor-Cmd Python import gate.
Does not rebuild earlier meshes, animation, sounds or accepted material graphs.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
INVENTORY = json.loads((ROOT / 'ArtSource/Weapons/Candidate03/presentation_inventory.json').read_text())
TOOLS = u.AssetToolsHelpers.get_asset_tools()
LIB = u.EditorAssetLibrary
MEL = u.MaterialEditingLibrary
REPORT = {'candidate': '03', 'stage': 'C', 'static_meshes': {}, 'materials': {}}

name = 'M_MuzzleFlash_C03'
destination = '/Game/ONE/Materials'
LIB.make_directory(destination)
material = LIB.load_asset(destination + '/' + name) if LIB.does_asset_exist(destination + '/' + name) else TOOLS.create_asset(name, destination, u.Material, u.MaterialFactoryNew())
MEL.delete_all_material_expressions(material)
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
material.set_editor_property('two_sided', True)
material.set_editor_property('disable_depth_test', False)
vertex = MEL.create_material_expression(material, u.MaterialExpressionVertexColor, -600, 0)
graph_branches = []
graph_connections = []


def require_connection(ok, label):
    if not ok:
        raise RuntimeError('Flash material graph connection rejected: ' + label)
    graph_connections.append({'connection': label, 'connected': True})


for parameter, value, output, prop, y in (
        # UE's unnamed aggregate vertex-colour output is addressed by an empty
        # name. "RGB" is not an accepted alias in GetExpressionOutputIndexByName.
        ('FlashGain', 12., '', u.MaterialProperty.MP_EMISSIVE_COLOR, 0),
        ('FlashAlpha', 0., 'A', u.MaterialProperty.MP_OPACITY, 240)):
    scalar = MEL.create_material_expression(material, u.MaterialExpressionScalarParameter, -600, y + 110)
    scalar.set_editor_property('parameter_name', parameter)
    scalar.set_editor_property('default_value', value)
    multiply = MEL.create_material_expression(material, u.MaterialExpressionMultiply, -250, y)
    require_connection(MEL.connect_material_expressions(vertex, output, multiply, 'A'),
                       'VertexColor.' + (output or 'aggregate') + ' -> ' + parameter + ' Multiply.A')
    require_connection(MEL.connect_material_expressions(scalar, '', multiply, 'B'),
                       parameter + ' -> Multiply.B')
    require_connection(MEL.connect_material_property(multiply, '', prop),
                       parameter + ' Multiply -> ' + str(prop))
    graph_branches.append((parameter, prop, multiply, scalar, output))
MEL.recompile_material(material)
graph_readback = []
for parameter, prop, multiply, scalar, output in graph_branches:
    actual_root = MEL.get_material_property_input_node(material, prop)
    names = list(MEL.get_material_expression_input_names(multiply))
    nodes = list(MEL.get_inputs_for_material_expression(material, multiply))
    inputs = dict(zip(names, nodes))
    if actual_root != multiply or inputs.get('A') != vertex or inputs.get('B') != scalar:
        raise RuntimeError('Flash material post-recompile graph readback failed: ' + parameter)
    graph_readback.append({'branch': parameter, 'property_input': actual_root.get_name(),
        'multiply_A': inputs['A'].get_name(), 'multiply_B': inputs['B'].get_name(),
        'requested_vertex_output': output or 'aggregate (index 0)', 'status': 'PASS'})
if not LIB.save_loaded_asset(material):
    raise RuntimeError('Unable to save corrected flash material')
REPORT['materials'][name] = {'asset': material.get_path_name(), 'source': 'Scripts/import_candidate03_weapon_presentation.py',
    'connections': graph_connections, 'post_recompile_graph_readback': graph_readback, 'graph_status': 'PASS'}
u.log('ONE C03 FLASH GRAPH PASS: six checked connections and both branches verified after recompile')

u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
for name, row in INVENTORY['static_meshes'].items():
    file = ROOT / row['source']
    digest = hashlib.sha256(file.read_bytes()).hexdigest()
    if digest != row['source_sha256']:
        raise RuntimeError('Source differs from authored inventory: ' + name)
    options = u.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_materials = False
    options.import_textures = False
    options.import_as_skeletal = False
    options.import_mesh = True
    options.import_animations = False
    options.mesh_type_to_import = u.FBXImportType.FBXIT_STATIC_MESH
    data = options.static_mesh_import_data
    for key, value in {'combine_meshes': True, 'auto_generate_collision': False,
            'generate_lightmap_u_vs': False, 'convert_scene': True,
            'convert_scene_unit': True, 'force_front_x_axis': False,
            'normal_import_method': u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS,
            'vertex_color_import_option': u.VertexColorImportOption.REPLACE}.items():
        data.set_editor_property(key, value)
    if LIB.does_asset_exist(row['asset']):
        existing = LIB.load_asset(row['asset'])
        saved = existing.get_editor_property('asset_import_data')
        for key in ('force_front_x_axis', 'convert_scene', 'convert_scene_unit'):
            saved.set_editor_property(key, data.get_editor_property(key))
        LIB.save_loaded_asset(existing)
    task = u.AssetImportTask()
    task.filename = str(file)
    task.destination_path = row['asset'].rsplit('/', 1)[0]
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    task.options = options
    task.factory = u.FbxFactory()
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(row['asset'])
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError('No imported StaticMesh: ' + name)
    for index, slot in enumerate(mesh.static_materials):
        slot_material = LIB.load_asset('/Game/ONE/Materials/' + str(slot.material_slot_name))
        if not slot_material:
            raise RuntimeError('Missing reused material ' + str(slot.material_slot_name))
        mesh.set_material(index, slot_material)
    LIB.save_loaded_asset(mesh)
    bounds = mesh.get_bounding_box()
    dimensions = [bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z]
    if any(abs(a-b) > .02 for a, b in zip(dimensions, row['dimensions_cm'])):
        raise RuntimeError('Case dimensions changed during import: ' + str(dimensions))
    REPORT['static_meshes'][name] = {'asset': mesh.get_path_name(), 'source': row['source'],
        'source_sha256': digest, 'dimensions_cm': dimensions}

REPORT['status'] = 'PASS'
report = ROOT / 'Saved/Candidate03/WeaponPresentationImport.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(REPORT, indent=2) + '\n')
u.log('ONE C03 PRESENTATION IMPORT PASS: 1 brass mesh and 1 original attached-flash material')
