"""Build the authored Project ONE 24 x 20m containment arena in Unreal 5.7.
Camera faces -Y from +Y. Scene dimensions, lighting and spawns are reproducible.
Run only after original assets have been imported with import_assets.py.
"""
from pathlib import Path
import json
import unreal as u
ROOT=Path(__file__).resolve().parents[1]
LEVEL='/Game/ONE/Maps/Containment'
ELL=u.EditorLevelLibrary
LIB=u.EditorAssetLibrary
ACTORS=u.get_editor_subsystem(u.EditorActorSubsystem)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
if LIB.does_asset_exist(LEVEL):
    if not LEVELS.load_level(LEVEL): raise RuntimeError('Cannot load existing generated map')
    for actor in ACTORS.get_all_level_actors():
        if isinstance(actor,u.WorldSettings) or actor.get_name()=='Brush_0': continue
        ACTORS.destroy_actor(actor)
else:
    if not LEVELS.new_level(LEVEL): raise RuntimeError('Cannot create generated map')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()

def prop(obj,name,value):
    try: obj.set_editor_property(name,value)
    except Exception as ex: u.log_warning('ONE map optional '+name+': '+str(ex))
def spawn(cls,name,loc=(0,0,0),rot=(0,0,0)):
    a=ACTORS.spawn_actor_from_class(cls,u.Vector(*loc),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    a.set_actor_label(name);return a
def mesh(name,loc=(0,0,0),yaw=0,scale=(1,1,1),collision=True,label=None):
    asset=LIB.load_asset('/Game/ONE/Art/Environment/'+name)
    if not asset: raise RuntimeError('Missing asset '+name)
    # Verified FBX conversion preserves X and flips Blender Y. Authored facades
    # face Blender -Y / Unreal +Y; turn them into the requested layout direction.
    a=spawn(u.StaticMeshActor,label or name,loc,(0,yaw if name=='SM_FloorWayfinding' else yaw+180,0))
    c=a.static_mesh_component;c.set_static_mesh(asset)
    a.set_actor_scale3d(u.Vector(*scale))
    c.set_mobility(u.ComponentMobility.STATIC)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return a

# Ground remains continuous at world Z=0, expansion joints are shallow decals.
for x in (-1000,-600,-200,200,600,1000):
    for y in (-800,-400,0,400,800): mesh('SM_FloorModule',(x,y,0))

# The authored facade faces -Y, so back wall is rotated into the play space.
for x in (-1000,-600,-200,200,600,1000):
    mesh('SM_WallBay',(x,-1015,0),180)
    mesh('SM_CutawayBarrier',(x,1015,0),0)
for y in (-800,-400,0,400,800):
    # Side wall upper halves are omitted toward camera, a deliberate cutaway.
    part='SM_WallBay' if y < 400 else 'SM_CutawayBarrier'
    mesh(part,(-1215,y,0),90)
    mesh(part,(1215,y,0),-90)
mesh('SM_PressureDoor',(0,-979,0),180,label='B-07 sealed containment door')
mesh('SM_ContainmentSign',(0,-972,327),180,collision=False)
for x in (-800,-400,400,800): mesh('SM_WallLight',(x,-976,272),180,collision=False)
for y in (-500,200):
    mesh('SM_WallLight',(-1175,y,272),90,collision=False)
    mesh('SM_WallLight',(1175,y,272),-90,collision=False)

# Back service bay equipment, compact readable central cover and two wall desks.
for x in (-990,-850,840,995): mesh('SM_PowerRack',(x,-828,0),180)
for x in (-625,625): mesh('SM_PressureVessel',(x,-803,0),180)
mesh('SM_LabConsole',(-1105,-270,0),90,label='West diagnostics terminal')
mesh('SM_LabConsole',(1105,-270,0),-90,label='East diagnostics terminal')
mesh('SM_ResearchBench',(-435,-175,0),90,label='West sealed specimen bench')
mesh('SM_ResearchBench',(435,-175,0),90,label='East sealed specimen bench')
mesh('SM_ServiceHatch',(0,-350,0),collision=False)
mesh('SM_ServiceHatch',(-710,415,0),collision=False)
mesh('SM_ServiceHatch',(710,415,0),collision=False)
mesh('SM_FloorWayfinding',(0,0,2),collision=False,label='B-07 floor wayfinding')

start=spawn(u.PlayerStart,'Response entry',(0,0,105),(0,-90,0))
for i,(x,y) in enumerate(((-950,-700),(950,-700),(-950,650),(950,650))):
    a=spawn(u.TargetPoint,f'Infected entry {i+1}',(x,y,95))
    a.tags=['ONE_Spawn']

# Nav volume is created through the editor actor factory, which supplies a cube
# brush. Actual bounds are saved below to catch missing/empty volume geometry.
nav=spawn(u.NavMeshBoundsVolume,'ONE Navigation',(0,0,100))
nav.set_actor_scale3d(u.Vector(12.5,10.5,3))
origin,extent=nav.get_actor_bounds(False)
u.log('ONE NAV VOLUME bounds '+str(origin)+' '+str(extent))
if extent.x < 1000 or extent.y < 900:
    raise RuntimeError('Navigation volume brush absent or too small: '+str(extent))

# Large overhead key and cool fill provide readable, restrained industrial light.
key=spawn(u.DirectionalLight,'Overhead diffusion',(0,0,700),(-62,-34,0))
key.light_component.set_intensity(24)
key.light_component.set_light_color(u.LinearColor(.83,.91,1,1))
key.light_component.set_mobility(u.ComponentMobility.MOVABLE)
prop(key.light_component,'light_source_angle',8)
for x in (-850,0,850):
    for y in (-600,200,750):
        a=spawn(u.PointLight,'Ceiling diffuse fill',(x,y,440))
        c=a.point_light_component;c.set_mobility(u.ComponentMobility.MOVABLE)
        c.set_intensity_units(u.LightUnits.LUMENS)
        c.set_intensity(8500);c.set_attenuation_radius(1050)
        c.set_light_color(u.LinearColor(.68,.86,.91,1));c.set_cast_shadows(False)
        prop(c,'source_radius',85)
for x in (-1000,1000):
    a=spawn(u.PointLight,'Amber safety light',(x,-690,195))
    c=a.point_light_component;c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_intensity_units(u.LightUnits.LUMENS)
    c.set_intensity(1800);c.set_attenuation_radius(430)
    c.set_light_color(u.LinearColor(1,.40,.10,1));c.set_cast_shadows(False)

pp=spawn(u.PostProcessVolume,'Neutral facility grade')
pp.unbound=True
s=pp.settings
for n,v in {
 'override_auto_exposure_method':True,'auto_exposure_method':u.AutoExposureMethod.AEM_MANUAL,
 'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False,
 'override_auto_exposure_min_brightness':True,'auto_exposure_min_brightness':4.,
 'override_auto_exposure_max_brightness':True,'auto_exposure_max_brightness':4.,
 'override_auto_exposure_bias':True,'auto_exposure_bias':-4.7,
 'override_bloom_intensity':True,'bloom_intensity':.08,
 'override_vignette_intensity':True,'vignette_intensity':.10,
 'override_motion_blur_amount':True,'motion_blur_amount':0.,
 'override_ambient_occlusion_intensity':True,'ambient_occlusion_intensity':.42,
 'override_ambient_occlusion_radius':True,'ambient_occlusion_radius':90.,
}.items(): prop(s,n,v)
pp.settings=s
settings=world.get_world_settings()
game=u.load_class(None,'/Script/ProjectONE.ONEGameMode')
if game: settings.set_editor_property('default_game_mode',game)
prop(settings,'force_no_precomputed_lighting',True)
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(0,1450,1550),u.Rotator(pitch=-57,yaw=-90,roll=0))
u.SystemLibrary.execute_console_command(world,'RebuildNavigation')
if not LEVELS.save_current_level(): raise RuntimeError('Cannot save generated map')
LIB.save_directory('/Game/ONE',only_if_is_dirty=False,recursive=True)
report={'level':LEVEL,'dimensions_cm':[2400,2000],
 'player_start':[0,0,105], 'camera_facing_yaw':-90,
 'nav_bounds_origin':str(origin),'nav_bounds_extent':str(extent),
 'actors':len(ACTORS.get_all_level_actors()),
 'original_static_assets':13,'lighting':'Movable key24lux, diffused fill8500lumens, amber1800lumens; manual exposure compensation -4.7 stops'}
(ROOT/'Evidence'/'map_generation.json').write_text(json.dumps(report,indent=2))
u.log('ONE MAP COMPLETE '+json.dumps(report))
