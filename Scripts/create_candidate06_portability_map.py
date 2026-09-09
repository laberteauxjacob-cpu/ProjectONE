"""Create only /Game/ONE/Maps/Portability06, a small relocated dependency fixture.

Run in the full editor via -ExecutePythonScript after the Candidate06 C++ build
and pickup imports. No Containment map, earlier asset or project default map is
changed. Runtime checks initialize the marked pickup through the real manager.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
LEVEL = '/Game/ONE/Maps/Portability06'
ORIGIN = (3200., -1700., 0.)
TAG = 'ONE06_PortabilityGenerated'
lib = u.EditorAssetLibrary
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
if lib.does_asset_exist(LEVEL):
    assert levels.load_level(LEVEL), 'Cannot load Portability06'
    # Repeatable generation owns only its tagged fixture actors. Legitimate
    # additions in this map and every other map remain untouched.
    for actor in list(actors.get_all_level_actors()):
        if TAG in [str(value) for value in actor.tags]:
            assert actors.destroy_actor(actor)
else:
    assert levels.new_level(LEVEL), 'Cannot create Portability06'
world = editor.get_editor_world()
generated = []


def location(offset):
    return u.Vector(*(ORIGIN[index] + offset[index] for index in range(3)))


def spawn(cls, label, offset, yaw=0., pitch=0., tags=()):
    actor = actors.spawn_actor_from_class(cls, location(offset), u.Rotator(pitch=pitch, yaw=yaw, roll=0))
    assert actor, label
    actor.set_actor_label(label)
    actor.tags = [u.Name(TAG)] + [u.Name(tag) for tag in tags]
    generated.append(actor)
    return actor


def static_mesh(asset_path, label, offset, scale=(1., 1., 1.), yaw=0., material=None, tags=()):
    mesh = lib.load_asset(asset_path)
    assert mesh, asset_path
    actor = spawn(u.StaticMeshActor, label, offset, yaw=yaw, tags=tags)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_mobility(u.ComponentMobility.STATIC)
    component.set_collision_profile_name('BlockAll')
    if material:
        loaded = lib.load_asset(material)
        assert loaded, material
        component.set_material(0, loaded)
    actor.set_actor_scale3d(u.Vector(*scale))
    return actor


for x in (-400., 0., 400.):
    for y in (-400., 0., 400.):
        static_mesh('/Game/ONE/Art/Environment/SM_FloorModule', 'Portability floor', (x, y, 0.), yaw=180.)
for offset, scale in (
    ((-610., 0., 45.), (.20, 12.4, .90)), ((610., 0., 45.), (.20, 12.4, .90)),
    ((0., -610., 45.), (12., .20, .90)), ((0., 610., 45.), (12., .20, .90)),
):
    static_mesh('/Engine/BasicShapes/Cube', 'Portability low edge', offset, scale,
                material='/Game/ONE/Materials/M_Graphite')
static_mesh('/Engine/BasicShapes/Cube', 'Portability grouping surface', (0., -510., 120.),
            (2.4, .12, 2.4), material='/Game/ONE/Materials/M_PaleMetal',
            tags=('Metal', 'ONE06_GroupingSurface'))

start = spawn(u.PlayerStart, 'Portability response start', (0., -100., 105.), yaw=-90.)
spawn_point = spawn(u.TargetPoint, 'Explicit test infected entry', (0., 310., 98.),
                    tags=('ONE_Spawn', 'ONE06_DeveloperSpawnFixture'))
pickup_point = spawn(u.TargetPoint, 'Manager-owned pickup test location', (125., 55., 12.),
                     tags=('ONE06_PickupFixture',))

placements = (
    ('ONEMysteryBox', 'Relocated Box - 950', (-345., -270., 2.), (3., 0., 52.), 35.),
    ('ONEUpgradeMachine', 'Relocated Pack-a-Punch - 5000', (335., -250., 2.), (26., 0., 110.), 145.),
)
machine_rows = []
for class_name, label, ground, center, yaw in placements:
    cls = u.load_class(None, '/Script/ProjectONE.' + class_name)
    assert cls, class_name
    angle = math.radians(yaw)
    offset = (ground[0] + center[0] * math.cos(angle) - center[1] * math.sin(angle),
              ground[1] + center[0] * math.sin(angle) + center[1] * math.cos(angle),
              ground[2] + center[2])
    machine = spawn(cls, label, offset, yaw=yaw, tags=('Metal', 'ONE06_PortabilityMachine'))
    collision = machine.get_component_by_class(u.BoxComponent)
    assert collision and collision.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION
    position = machine.get_actor_location()
    machine_rows.append({'class': class_name, 'actor_position_cm': [position.x, position.y, position.z],
                         'ground_position_cm': [ORIGIN[i] + ground[i] for i in range(3)], 'yaw_degrees': yaw})

nav = spawn(u.NavMeshBoundsVolume, 'Portability navigation', (0., 0., 100.))
nav.set_actor_scale3d(u.Vector(6.2, 6.2, 3.))
nav_origin, nav_extent = nav.get_actor_bounds(False)
assert nav_extent.x >= 600 and nav_extent.y >= 600 and nav_extent.z >= 100, 'Navigation brush missing/undersized'
assert abs(nav_origin.x - ORIGIN[0]) < 1 and abs(nav_origin.y - ORIGIN[1]) < 1, 'Navigation origin not relocated'

key = spawn(u.DirectionalLight, 'Portability overhead key', (0., 0., 700.), yaw=-34., pitch=-62.)
key.light_component.set_mobility(u.ComponentMobility.MOVABLE)
key.light_component.set_intensity(24.)
key.light_component.set_light_color(u.LinearColor(.83, .91, 1., 1.))
for x in (-300., 300.):
    light = spawn(u.PointLight, 'Portability soft fill', (x, 0., 420.))
    component = light.point_light_component
    component.set_mobility(u.ComponentMobility.MOVABLE)
    component.set_intensity_units(u.LightUnits.LUMENS)
    component.set_intensity(8500.)
    component.set_attenuation_radius(1000.)
    component.set_light_color(u.LinearColor(.68, .86, .91, 1.))
    component.set_cast_shadows(False)

post = spawn(u.PostProcessVolume, 'Portability neutral exposure', (0., 0., 0.))
post.unbound = True
settings = post.settings
for name, value in {
    'override_auto_exposure_method': True, 'auto_exposure_method': u.AutoExposureMethod.AEM_MANUAL,
    'override_auto_exposure_apply_physical_camera_exposure': True, 'auto_exposure_apply_physical_camera_exposure': False,
    'override_auto_exposure_min_brightness': True, 'auto_exposure_min_brightness': 4.,
    'override_auto_exposure_max_brightness': True, 'auto_exposure_max_brightness': 4.,
    'override_auto_exposure_bias': True, 'auto_exposure_bias': -4.7,
    'override_bloom_intensity': True, 'bloom_intensity': .08,
    'override_motion_blur_amount': True, 'motion_blur_amount': 0.,
}.items():
    settings.set_editor_property(name, value)
post.settings = settings
game_mode = u.load_class(None, '/Script/ProjectONE.ONEGameMode')
assert game_mode
world_settings = world.get_world_settings()
world_settings.set_editor_property('default_game_mode', game_mode)
world_settings.set_editor_property('force_no_precomputed_lighting', True)
editor.set_level_viewport_camera_info(location((0., 1150., 1300.)), u.Rotator(pitch=-57, yaw=-90, roll=0))
assert u.ONE03PhysicsAssets.rebuild_navigation_and_wait(world), 'Navigation must finish before saving'
assert levels.save_current_level(), 'Cannot save Portability06'

report = {
    'candidate': '06', 'status': 'MAP_GENERATED_NAVIGATION_BUILT_AND_SAVED',
    'map': LEVEL, 'origin_cm': list(ORIGIN), 'floor_dimensions_cm': [1200, 1200],
    'game_mode': '/Script/ProjectONE.ONEGameMode', 'generated_actor_count': len(generated),
    'player_start_cm': [start.get_actor_location().x, start.get_actor_location().y, start.get_actor_location().z],
    'spawn_fixture_cm': [spawn_point.get_actor_location().x, spawn_point.get_actor_location().y, spawn_point.get_actor_location().z],
    'pickup_fixture_cm': [pickup_point.get_actor_location().x, pickup_point.get_actor_location().y, pickup_point.get_actor_location().z],
    'machines': machine_rows,
    'navigation_extent_cm': [nav_extent.x, nav_extent.y, nav_extent.z],
    'source': 'Scripts/create_candidate06_portability_map.py',
    'source_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'functional_validation_performed': False,
    'limits': ['Developer-only dependency fixture; visible spawn marker is not production hidden-entry design.',
               'Runtime checks must create the pickup through the normal manager at ONE06_PickupFixture.',
               'Generation and navigation save do not establish health, scoring, return/expiry or visual validation.'],
}
output = ROOT / 'Evidence/Candidate06/PortabilityMapGeneration.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
u.log('ONE06_PORTABILITY_MAP_GENERATED ' + json.dumps(report))
