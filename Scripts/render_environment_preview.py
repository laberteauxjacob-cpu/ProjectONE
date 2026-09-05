"""Blender-only model inspection. These images are NOT runtime evidence."""
from pathlib import Path
import bpy
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'ArtSource'/'Environment'/'ProjectONE_IndustrialKit.blend'))
s=bpy.context.scene
for o in s.objects: o.hide_render=True
weapon=bpy.data.objects['SM_Carbine'];weapon.location=(0,0,0);weapon.hide_render=False
def point(o,target): o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(86,-118,68))
cam=bpy.context.object;point(cam,(7,0,8));cam.data.type='ORTHO';cam.data.ortho_scale=116;s.camera=cam
for pos,power,size in (((30,-75,130),400000,100),((-50,20,50),190000,70),((65,65,95),350000,100)):
    bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;point(o,(0,0,8))
s.world.use_nodes=True;s.world.node_tree.nodes['Background'].inputs[0].default_value=(.25,.29,.30,1)
s.world.node_tree.nodes['Background'].inputs[1].default_value=.6
s.render.engine='CYCLES';s.cycles.samples=32
s.render.resolution_x=1600;s.render.resolution_y=1000;s.render.resolution_percentage=100
s.render.image_settings.file_format='PNG';s.render.filepath=str(ROOT/'Evidence'/'Blender_Carbine_Inspection.png')
bpy.ops.render.render(write_still=True)
