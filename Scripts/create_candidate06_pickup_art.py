"""Root-scheduled Unreal material authoring for the three original C06 emblems.

The editable mesh composition is ONE06PickupVisualComponent.cpp (owned engine
primitives); HUD counterparts are Canvas geometry in ONEHUD.cpp. No external
image, model, font, recording or Project Zero input is read by this script.
Run inside the project's Unreal Python environment; this does not launch UE.
"""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
DESTINATION = '/Game/ONE/Pickups/Candidate06'
ASSET = DESTINATION + '/M_Pickup06'
lib = u.EditorAssetLibrary
mel = u.MaterialEditingLibrary
lib.make_directory(DESTINATION)
material = lib.load_asset(ASSET) if lib.does_asset_exist(ASSET) else u.AssetToolsHelpers.get_asset_tools().create_asset(
    'M_Pickup06', DESTINATION, u.Material, u.MaterialFactoryNew())
assert isinstance(material, u.Material)
mel.delete_all_material_expressions(material)
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('two_sided', True)
material.set_editor_property('disable_depth_test', False)
tint = mel.create_material_expression(material, u.MaterialExpressionVectorParameter, -500, 0)
tint.set_editor_property('parameter_name', 'Tint')
tint.set_editor_property('default_value', u.LinearColor(.03, .82, .60, 1))
emission = mel.create_material_expression(material, u.MaterialExpressionScalarParameter, -500, 160)
emission.set_editor_property('parameter_name', 'Emission')
emission.set_editor_property('default_value', .65)
opacity = mel.create_material_expression(material, u.MaterialExpressionScalarParameter, -500, 300)
opacity.set_editor_property('parameter_name', 'Opacity')
opacity.set_editor_property('default_value', 1.)
multiply = mel.create_material_expression(material, u.MaterialExpressionMultiply, -230, 0)
assert mel.connect_material_expressions(tint, '', multiply, 'A')
assert mel.connect_material_expressions(emission, '', multiply, 'B')
assert mel.connect_material_property(multiply, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
assert mel.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
mel.recompile_material(material)
assert mel.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR) == multiply
assert mel.get_material_property_input_node(material, u.MaterialProperty.MP_OPACITY) == opacity
assert lib.save_loaded_asset(material)
sources = ['Scripts/create_candidate06_pickup_art.py', 'ArtSource/Pickups/Candidate06/design.json',
           'Source/ProjectONE/ONE06PickupVisualComponent.cpp', 'Source/ProjectONE/ONEHUD.cpp']
report = {
    'candidate': '06', 'status': 'MATERIAL_CREATED_AND_GRAPH_CHECKED', 'asset': ASSET,
    'depth_test': True, 'external_inputs': [], 'visual_approval': False,
    'source_files': [{'file': name, 'sha256': hashlib.sha256((ROOT / name).read_bytes()).hexdigest()} for name in sources],
    'scope': 'Material graph and original editable composition inventory, not an in-engine visual review.',
}
output = ROOT / 'Evidence/Candidate06/PickupArtImport.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
u.log('ONE06_PICKUP_ART_IMPORT_PASS one depth-tested parameter material; three original procedural emblems')
