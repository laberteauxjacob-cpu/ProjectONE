"""Read imported asset scale, reference transforms and exact clip availability."""
from pathlib import Path
import json,unreal as u
ROOT=Path(__file__).resolve().parents[1]
LIB=u.EditorAssetLibrary
ACT=u.get_editor_subsystem(u.EditorActorSubsystem)
report={}
for name in ('SM_Carbine','SM_FloorModule','SM_WallBay','SM_FloorWayfinding'):
    m=LIB.load_asset('/Game/ONE/Art/Environment/'+name)
    b=m.get_bounds()
    report[name]={'origin_cm':str(b.origin),'extent_cm':str(b.box_extent),'materials':[str(s.material_slot_name) for s in m.static_materials]}
for name in ('SK_Response','SK_Infected','SK_Infected_Head','SK_Infected_ArmL'):
    m=LIB.load_asset('/Game/ONE/Characters/'+name)
    actor=ACT.spawn_actor_from_class(u.SkeletalMeshActor,u.Vector(0,0,0))
    comp=actor.skeletal_mesh_component;comp.set_skeletal_mesh_asset(m)
    bones={str(b):str(comp.get_socket_transform(b,u.RelativeTransformSpace.RTS_COMPONENT)) for b in ('root','head','hand_l','hand_r','weapon_r')}
    report[name]={'skeleton':m.skeleton.get_name(),'num_bones':comp.get_num_bones(),'ref_bones':bones,'bounds':str(actor.get_actor_bounds(False))}
    ACT.destroy_actor(actor)
report['animations']={}
for file in sorted((ROOT/'ArtSource'/'Exports').glob('A_*.fbx')):
    a=LIB.load_asset('/Game/ONE/Animations/'+file.stem)
    if not a: raise RuntimeError('Missing clip '+file.stem)
    report['animations'][file.stem]=a.get_play_length()
(ROOT/'Evidence'/'unreal_asset_audit.json').write_text(json.dumps(report,indent=2))
u.log('ONE ASSET AUDIT '+json.dumps(report))
