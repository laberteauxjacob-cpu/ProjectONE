"""Read-only persisted placement/visibility check for floor mesh debugging."""
from pathlib import Path
import unreal as u,json
ROOT=Path(__file__).resolve().parents[1]
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/ONE/Maps/Containment')
rows=[];floor_tops=[];wall_tops=[];wayfinding_min=None
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if isinstance(a,u.StaticMeshActor):
        c=a.static_mesh_component;m=c.static_mesh
        if m and ('Floor' in m.get_name() or 'Hatch' in m.get_name() or m.get_name()=='SM_WallBay'):
            rows.append({'label':a.get_actor_label(),'mesh':m.get_name(),'loc':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation()),'bounds':str(a.get_actor_bounds(False)),'visible':c.is_visible(),'hidden':a.get_editor_property('hidden'),'component_hidden':c.get_editor_property('hidden_in_game'),'materials':[str(c.get_material(i)) for i in range(c.get_num_materials())]})
            rot=a.get_actor_rotation();center,extent=a.get_actor_bounds(False)
            if m.get_name()=='SM_FloorModule':
                assert abs(rot.pitch)<.001 and abs(rot.roll)<.001,'Floor is tilted: '+str(rot)
                floor_tops.append(center.z+extent.z)
                assert 0<=floor_tops[-1]<=2,'Floor surface height incorrect'
            elif m.get_name()=='SM_WallBay':
                assert abs(rot.pitch)<.001 and abs(rot.roll)<.001,'Wall is tilted'
                wall_tops.append(center.z+extent.z)
                assert 299<=wall_tops[-1]<=301,'Wall height incorrect'
            elif m.get_name()=='SM_FloorWayfinding':
                wayfinding_min=center.z-extent.z
assert len(floor_tops)==30 and wall_tops,'Architectural modules absent'
assert wayfinding_min is not None and wayfinding_min>max(floor_tops),'Floor markings buried'
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
trace=u.SystemLibrary.line_trace_single(world_context_object=world,start=u.Vector(x=100,y=100,z=300),end=u.Vector(x=100,y=100,z=-100),trace_channel=u.TraceTypeQuery.TRACE_TYPE_QUERY1,trace_complex=False,actors_to_ignore=[],draw_debug_type=u.DrawDebugTrace.NONE,ignore_self=True)
u.log('ONE FLOOR TRACE '+str(trace))
hit=next((p for p in trace if isinstance(p,u.HitResult)),None) if isinstance(trace,tuple) else trace
assert isinstance(hit,u.HitResult),'Floor trace missing'
parts=hit.to_tuple()
impact=parts[5]
assert parts[0] and 0<=impact.z<=2,'Collision trace floor height incorrect: '+str(parts)
u.log('ONE FLOOR TRACE Z '+str(impact.z))
rows.append({'assertions':'30 floors horizontal at0..2cm; full walls upright at300cm; all wayfinding bounds above floor; blocking collision trace at0..2cm','floor_tops_cm':floor_tops,'wall_tops_cm':wall_tops,'wayfinding_min_cm':wayfinding_min,'floor_trace_impact_z_cm':impact.z})
(ROOT/'Evidence'/'map_placement_audit.json').write_text(json.dumps(rows,indent=2))
u.log('ONE PLACEMENT '+json.dumps(rows))
