"""Author the original spent rifle case only; never reads or changes C01/C02 art.

Blender 5.1: --background --python Scripts/create_candidate03_weapon_presentation.py
Centimetres, local +X case axis, origin at centre. The native attached muzzle
geometry is authored in ONEPlayer::BuildMuzzleFlash; its material is imported by
import_candidate03_weapon_presentation.py. No downloaded models or textures.
"""
from pathlib import Path
import hashlib
import json
import math
import bpy
import bmesh

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ArtSource/Weapons/Candidate03'
SOURCE.mkdir(parents=True, exist_ok=True)
NAME = 'SM_RifleBrass_C03'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = .01

material = bpy.data.materials.new('M_Brass')
material.use_nodes = True
material.diffuse_color = (.46, .31, .095, 1)
shader = material.node_tree.nodes.get('Principled BSDF')
shader.inputs['Base Color'].default_value = material.diffuse_color
shader.inputs['Roughness'].default_value = .4
shader.inputs['Metallic'].default_value = .75

# Lathed rim, extractor groove, tapered case body, shoulder and open neck.
# This is a spent case: no bullet or pointed projectile is present.
profile = [(-2.25, .475), (-2.16, .475), (-2.13, .39), (-2.00, .39),
           (-1.92, .46), (.97, .445), (1.34, .43), (1.62, .30),
           (2.25, .30), (2.25, .245), (1.64, .245), (1.40, .29)]
sides = 24
vertices = [(x, radius * math.cos(2 * math.pi * i / sides),
             radius * math.sin(2 * math.pi * i / sides))
            for x, radius in profile for i in range(sides)]
faces = []
for ring in range(len(profile) - 1):
    for i in range(sides):
        a, b = ring * sides + i, ring * sides + (i + 1) % sides
        faces.append((a, b, b + sides, a + sides))
faces.append(tuple(reversed(range(sides))))
faces.append(tuple((len(profile) - 1) * sides + i for i in range(sides)))
mesh = bpy.data.meshes.new(NAME)
mesh.from_pydata(vertices, [], faces)
mesh.materials.append(material)
mesh.update()
obj = bpy.data.objects.new(NAME, mesh)
scene.collection.objects.link(obj)
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
bm = bmesh.new()
bm.from_mesh(mesh)
bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
bm.to_mesh(mesh)
bm.free()
for polygon in mesh.polygons:
    polygon.use_smooth = len(polygon.vertices) == 4
# Subtle authored heat/soot variation at the mouth and interior, using existing
# vertex-colour-aware brass material. No image texture dependency.
colors = mesh.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
for polygon in mesh.polygons:
    for loop in polygon.loop_indices:
        index = mesh.loops[loop].vertex_index
        inner = index // sides >= 9
        mouth = vertices[index][0] > 1.6
        tint = (.16, .12, .075, 1) if inner else ((.61, .46, .25, 1) if mouth else (1, 1, 1, 1))
        colors.data[loop].color = tint

bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE / 'RifleBrass_C03.blend'))
fbx = SOURCE / (NAME + '.fbx')
bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'MESH'},
    axis_forward='-Y', axis_up='Z', global_scale=1, apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_UNITS', use_space_transform=True,
    bake_space_transform=False, mesh_smooth_type='FACE', bake_anim=False)
inventory = {
    'candidate': '03', 'stage': 'C', 'generator': 'Scripts/create_candidate03_weapon_presentation.py',
    'provenance': 'Original project-local procedural mesh and material graph; no external model, image or sample input',
    'coordinate_convention': 'Centimetres, case cylinder axis +X; centred origin; UE legacy import reflects source Y',
    'static_meshes': {NAME: {
        'source': str(fbx.relative_to(ROOT)).replace('\\', '/'),
        'blend': 'ArtSource/Weapons/Candidate03/RifleBrass_C03.blend',
        'asset': '/Game/ONE/Art/Weapons/' + NAME,
        'source_sha256': hashlib.sha256(fbx.read_bytes()).hexdigest(),
        'dimensions_cm': [4.5, .95, .95], 'material_slots': ['M_Brass'],
        'description': 'Spent bottleneck rifle case, extractor groove, open neck; no projectile',
    }},
    'materials': {'M_MuzzleFlash_C03': {
        'source': 'Scripts/import_candidate03_weapon_presentation.py',
        'asset': '/Game/ONE/Materials/M_MuzzleFlash_C03',
        'shading': 'Unlit additive, two-sided, ordinary depth testing',
        'emissive': 'Vertex RGB multiplied by FlashGain (12)',
        'opacity': 'Vertex alpha multiplied by FlashAlpha (default 0; animated per discharge)',
        'geometry_source': 'Source/ProjectONE/ONEPlayer.cpp: BuildMuzzleFlash',
        'texture_dependencies': [],
    }},
    'reused_assets': ['/Game/ONE/Materials/M_Brass', '/Game/ONE/Art/Weapons/SM_ShotgunShell'],
    'validation': {'source_dimensions_cm': list(obj.dimensions), 'vertices': len(mesh.vertices),
                   'polygons': len(mesh.polygons), 'external_inputs': []},
}
(SOURCE / 'presentation_inventory.json').write_text(json.dumps(inventory, indent=2) + '\n')
assert all(abs(a-b) < .001 for a, b in zip(obj.dimensions, (4.5, .95, .95)))
print('ONE C03 PRESENTATION SOURCE PASS: original spent brass, 4.5 x .95 x .95 cm')
