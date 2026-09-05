"""Regenerate stale original-kit collision, then fully build and save navigation.

Run through UnrealEditor -ExecutePythonScript after compiling ProjectONEEditor.
StaticMeshEditorSubsystem must be initialized (the Python commandlet omits it).
The existing arena layout and rendered/source mesh geometry are preserved.
The report compares actual simple/nav convex vertices with current render bounds.
"""
from pathlib import Path
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
LEVEL='/Game/ONE/Maps/Containment'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
meshes=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
assert meshes, 'Run in the full editor with -ExecutePythonScript; StaticMeshEditorSubsystem is unavailable'
assert levels.load_level(LEVEL), 'Existing containment map must load'
before=json.loads(u.ONE03PhysicsAssets.inspect_environment_collision())
for row in before['assets']:
    mesh=u.load_asset('/Game/ONE/Art/Environment/'+row['mesh'])
    assert mesh and meshes.remove_collisions(mesh), row['mesh']
    assert meshes.add_simple_collisions(mesh,u.ScriptCollisionShapeType.NDOP18)>=0, row['mesh']
    assert meshes.get_simple_collision_count(mesh)==0 and meshes.get_convex_collision_count(mesh)==1, row['mesh']
    assert u.EditorAssetLibrary.save_loaded_asset(mesh), row['mesh']
after=json.loads(u.ONE03PhysicsAssets.inspect_environment_collision())
for old,new in zip(before['assets'],after['assets']):
    assert old['mesh']==new['mesh']
    assert max(abs(a-b) for key in ('render_min_cm','render_max_cm') for a,b in zip(old[key],new[key]))<.001
    assert new['simple_shape_count']==1 and len(new['convex_hulls'])==1
    assert new['convex_hulls'][0]['outside_render_bounds_2cm']==0
    assert new['nav_convex']['outside_render_bounds_2cm']==0
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
nav=[]
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,u.RecastNavMesh):
        prior={key:float(actor.get_editor_property(key)) for key in ('agent_radius','agent_height')}
        actor.set_editor_property('agent_radius',32.0)
        actor.set_editor_property('agent_height',185.0)
        nav.append({'previous':prior,'current':{key:float(actor.get_editor_property(key)) for key in prior}})
assert nav and u.ONE03PhysicsAssets.rebuild_navigation_and_wait(world), 'Navigation did not finish building'
assert levels.save_current_level(), 'Corrected navigation must be saved'
report={'status':'PASS','source':'Scripts/repair_candidate03_environment_collision.py',
  'method':'Clear previous simple hulls; generate one NDOP18 from current render geometry; refresh navigation collision; wait for a full navigation build before saving.',
  'render_geometry_bounds_tolerance_cm':.001,'render_geometry_bounds_unchanged':True,'layout_unchanged':True,'navigation_build_completed_before_save':True,
  'before':before,'after':after,'navigation_agents':nav,
  'runtime_scope':'Geometric correction verified here; corner/rack pursuit and rendered corpse contact require separate gameplay checks.'}
out=ROOT/'Evidence/Candidate03/StageD/EnvironmentCollisionRepair.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
u.log('ONE03_ENVIRONMENT_COLLISION_REPAIR_COMPLETE')
