"""Add the two C04 physical machines to the existing arena and save rebuilt nav.

Run in the full editor via -ExecutePythonScript after C04 imports/build. Existing
arena actors, geometry, lights, collision assets and spawn markers are retained.
"""
from pathlib import Path
import json
import math
import unreal as u

root=Path(__file__).resolve().parents[1]
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/ONE/Maps/Containment')
def is_machine(actor):
    return actor.get_class().get_name() in ('ONEMysteryBox','ONEUpgradeMachine')
def numeric_transform(actor):
    p=actor.get_actor_location(); r=actor.get_actor_rotation(); s=actor.get_actor_scale3d()
    return [p.x,p.y,p.z,r.pitch,r.yaw,r.roll,s.x,s.y,s.z]
before={a.get_path_name():numeric_transform(a) for a in actors.get_all_level_actors() if not is_machine(a)}
for actor in list(actors.get_all_level_actors()):
    if is_machine(actor):
        assert actors.destroy_actor(actor)
placements=[
    ('ONEMysteryBox','Mystery Box - 950',(-650,-530,2),(3,0,52),0),
    ('ONEUpgradeMachine','Pack-a-Punch - 5000',(650,-530,2),(26,0,110),180),
]
rows=[]
for class_name,label,ground,center,yaw in placements:
    cls=u.load_class(None,'/Script/ProjectONE.'+class_name)
    assert cls,class_name
    rotation=u.Rotator(pitch=0,yaw=yaw,roll=0)
    angle=math.radians(yaw)
    offset=u.Vector(center[0]*math.cos(angle)-center[1]*math.sin(angle),center[0]*math.sin(angle)+center[1]*math.cos(angle),center[2])
    position=u.Vector(*ground)+offset
    actor=actors.spawn_actor_from_class(cls,position,rotation)
    assert actor,class_name
    actor.set_actor_label(label)
    actor.tags=[u.Name('Metal'),u.Name('ONE_C04_Machine')]
    box=actor.get_component_by_class(u.BoxComponent)
    assert box and box.get_collision_enabled()!=u.CollisionEnabled.NO_COLLISION
    extent=box.get_unscaled_box_extent()
    rows.append({'class':class_name,'ground_root_cm':ground,'actor_center_cm':[position.x,position.y,position.z],
                 'yaw':yaw,'collision_half_extent_cm':[extent.x,extent.y,extent.z]})
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert u.ONE03PhysicsAssets.rebuild_navigation_and_wait(world),'Navigation build must finish before save'
after={a.get_path_name():numeric_transform(a) for a in actors.get_all_level_actors() if not is_machine(a)}
assert before.keys()==after.keys(),'An existing arena actor was added or removed'
for key,transform in before.items():
    assert max(abs(a-b) for a,b in zip(transform,after[key]))<.0001,'An existing actor moved: '+key
assert levels.save_current_level(),'Could not save C04 machines/navigation'
report=root/'Saved/Candidate04/MachinePlacement.json'
report.parent.mkdir(parents=True,exist_ok=True)
report.write_text(json.dumps({'status':'PASS','preserved_existing_actors':len(before),'navigation_saved_after_build':True,'machines':rows},indent=2)+'\n')
u.log('ONE04_MACHINE_PLACEMENT_PASS')
