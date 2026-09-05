"""Run inside Unreal Editor after building the native authoring helper.

Constructs deterministic explicit physics bodies from the accepted complete
reference skeleton. Runtime validation is separate from successful generation.
"""
import unreal

if not unreal.ONE03PhysicsAssets.build_infected_assets():
    raise RuntimeError('Candidate03 physics authoring failed; inspect the Unreal log')
unreal.log('ONE03_PHYSICS_AUTHORING_COMPLETE')
