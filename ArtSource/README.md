# Project ONE editable art sources

Candidate07's editable character, motion and audio sources are indexed below.
[CurrentState](../Docs/CurrentState.md) and the [pass report](../Docs/Passes/Candidate07.md)
record implementation, verification and publication separately from user
approval of the visual/audio direction.

| Candidate07 source | Current contents | Editing and provenance |
| --- | --- | --- |
| [Infected trio](Characters/Candidate07/README.md) | Maintenance, Laboratory and FacilityStaff: 3 character Blender files, 15 modular mesh FBXs and 2 shared original normal-map PNGs | [Generator](../Scripts/create_candidate07_infected.py), per-appearance design/inventory/validation files and [head fit](Characters/Candidate07/physics_fit.json) |
| [Infected motion](Characters/Candidate07/Motion/inventory.json) | 1 editable motion Blender file and 14 animation FBXs, including separate prone and supine recoveries | [Motion generator](../Scripts/create_candidate07_infected_motion.py); partial regeneration preserves the other actions/exports and verifies the saved action bank |
| [Recorded audio](Audio/Candidate07/README.md) | 76 retained source files (38,893,854 bytes) and 134 processed mono 48 kHz PCM cues | [Sources](Audio/Candidate07/source_inventory.json), [credits](Audio/Candidate07/CREDITS.md), [recipes](Audio/Candidate07/recipes.json), [output manifest](Audio/Candidate07/manifest.json) and [processor](../Scripts/process_candidate07_audio.py) |

The trio uses the existing infected bind rig and five-part cut contract. The
player's separate character/animation sources remain retained. Physical audio
uses recordings, with documented Foley and firearm analogues; intentional
upgraded energy layers remain synthesized. Audio licenses apply to the listed
sources only. See [provenance](../Docs/Provenance.md) for the broader source record.

Use the [character guide](../Docs/CharacterPipeline.md#candidate07-trio-and-infected-only-motion)
and [targeted import order](../Docs/EnvironmentPipeline.md#candidate07-targeted-import-order):
author and verify sources, sanitize changed Blender/FBX metadata and refresh
only proven metadata hashes, compile the editor helper, import Maintenance then
the other two appearances, populate their motion maps, and import verified audio
with commandlet audio enabled. Generation, source validation, import, runtime
tests, frame review, listening, native input and packaged verification are
separate results.

Historical sources remain available: [original character pipeline](../Docs/CharacterPipeline.md),
[original environment/firearm pipeline](../Docs/EnvironmentPipeline.md),
[Candidate03 modular infected](Characters/Candidate03/README.md),
[Candidate05 motion](Characters/C05/README.md),
[Candidate04 weapons](Weapons/Candidate04/README.md),
[Candidate04 machines](Machines/Candidate04/README.md) and
[Candidate06 pickups](Pickups/Candidate06/README.md). Use their candidate-specific
instructions for historical regeneration; they do not replace the C07 importers.
