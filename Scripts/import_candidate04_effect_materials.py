"""Create the vertex-colored bounded C04 tracer material; earlier effects stay intact."""
from pathlib import Path
import json
import unreal as u

root=Path(__file__).resolve().parents[1]
lib=u.EditorAssetLibrary
mel=u.MaterialEditingLibrary
name='M_Tracer_C04'
destination='/Game/ONE/Materials'
asset=destination+'/'+name
material=lib.load_asset(asset) if lib.does_asset_exist(asset) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,destination,u.Material,u.MaterialFactoryNew())
mel.delete_all_material_expressions(material)
material.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
material.set_editor_property('two_sided',True)
material.set_editor_property('disable_depth_test',False)
color=mel.create_material_expression(material,u.MaterialExpressionVertexColor,-500,0)
gain=mel.create_material_expression(material,u.MaterialExpressionConstant,-500,180)
gain.set_editor_property('r',4.)
multiply=mel.create_material_expression(material,u.MaterialExpressionMultiply,-240,0)
assert mel.connect_material_expressions(color,'',multiply,'A')
assert mel.connect_material_expressions(gain,'',multiply,'B')
assert mel.connect_material_property(multiply,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
assert mel.connect_material_property(color,'A',u.MaterialProperty.MP_OPACITY)
mel.recompile_material(material)
assert mel.get_material_property_input_node(material,u.MaterialProperty.MP_EMISSIVE_COLOR)==multiply
assert mel.get_material_property_input_node(material,u.MaterialProperty.MP_OPACITY)==color
assert lib.save_loaded_asset(material)
report=root/'Saved/Candidate04/EffectMaterialImport.json'
report.parent.mkdir(parents=True,exist_ok=True)
report.write_text(json.dumps({'status':'PASS','asset':asset,'gain':4,'depth_test':True,'graph_connections_checked':4},indent=2)+'\n')
u.log('ONE04_EFFECT_MATERIAL_IMPORT_PASS')
