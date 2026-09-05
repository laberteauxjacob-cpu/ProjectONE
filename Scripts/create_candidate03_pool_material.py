"""Create a growing-pool decal from Project ONE's existing original blood mask.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>. No new bitmap
or external asset: resample the existing central pool, leaving the full impact
and wound mask unchanged. Verify every connection and compiled property roots.
"""
from pathlib import Path
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
LIB=u.EditorAssetLibrary
MEL=u.MaterialEditingLibrary
path='/Game/ONE/Materials/M_BloodPool_C03'
mask=LIB.load_asset('/Game/ONE/Textures/T_BloodMask')
if not isinstance(mask,u.Texture2D): raise RuntimeError('Original blood mask missing')
m=LIB.load_asset(path) if LIB.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_BloodPool_C03','/Game/ONE/Materials',u.Material,u.MaterialFactoryNew())
MEL.delete_all_material_expressions(m)
m.set_editor_property('material_domain',u.MaterialDomain.MD_DEFERRED_DECAL)
m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
def node(cls,x,y): return MEL.create_material_expression(m,cls,x,y)
def vec2(x,y,px,py):
    n=node(u.MaterialExpressionConstant2Vector,px,py)
    n.set_editor_property('r',x);n.set_editor_property('g',y)
    return n
connections=[]
def require(ok,label):
    if not ok: raise RuntimeError('Pool graph connection failed: '+label)
    connections.append(label)
uv=node(u.MaterialExpressionTextureCoordinate,-800,0)
scale=vec2(.55,.36,-800,180)
mul=node(u.MaterialExpressionMultiply,-570,0)
offset=vec2(.225,.32,-570,180)
add=node(u.MaterialExpressionAdd,-350,0)
sample=node(u.MaterialExpressionTextureSample,-120,0)
sample.texture=mask
sample.sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
for a,output,b,pin,label in [(uv,'',mul,'A','UV'),(scale,'',mul,'B','Scale'),(mul,'',add,'A','Scaled UV'),(offset,'',add,'B','Center offset'),(add,'',sample,'','Pool mask UV (first texture input)')]:
    require(MEL.connect_material_expressions(a,output,b,pin),label)
color=node(u.MaterialExpressionConstant3Vector,-250,-260)
color.set_editor_property('constant',u.LinearColor(.14,.004,.009,1))
rough=node(u.MaterialExpressionConstant,-250,-130);rough.set_editor_property('r',.55)
for n,output,prop in [(color,'',u.MaterialProperty.MP_BASE_COLOR),(rough,'',u.MaterialProperty.MP_ROUGHNESS),(sample,'A',u.MaterialProperty.MP_OPACITY)]:
    require(MEL.connect_material_property(n,output,prop),str(prop))
MEL.recompile_material(m)
if list(MEL.get_inputs_for_material_expression(m,sample))[0]!=add: raise RuntimeError('Compiled pool mask UV input differs')
for n,prop in [(color,u.MaterialProperty.MP_BASE_COLOR),(rough,u.MaterialProperty.MP_ROUGHNESS),(sample,u.MaterialProperty.MP_OPACITY)]:
    if MEL.get_material_property_input_node(m,prop)!=n: raise RuntimeError('Compiled pool graph root differs')
if not LIB.save_loaded_asset(m): raise RuntimeError('Cannot save pool material')
report={'status':'PASS','source':'Scripts/create_candidate03_pool_material.py','asset':path,'reused_original_mask':'/Game/ONE/Textures/T_BloodMask','uv_scale':[.55,.36],'uv_offset':[.225,.32],'reason':'Original central ellipse occupied only 47% by 30% of decal half-extents; pool-specific UVs use the central continuous shape at approximately85% by83%, while impact/wound presentation retains the complete splatter mask.','checked_connections':connections,'compiled_property_roots_verified':3}
(ROOT/'Evidence/Candidate03/StageD/PoolMaterial.json').write_text(json.dumps(report,indent=2)+'\n')
u.log('ONE03_POOL_MATERIAL_COMPLETE '+json.dumps(report))
