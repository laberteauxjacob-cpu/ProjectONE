# Project ONE — Candidate03 development

An original solo, top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate03 corrects movement, reload intent, weapon presentation and infected
physicality within the existing two-weapon containment arena.

**Candidate03 has no public release yet.** Internal stages B (movement/reload)
and C (weapon presentation) are complete. Stage D passed 244 rendered integration
checks plus focused freeze/resume motion review; stage E's fresh-checkout package, combined validation and
publication are pending. See [CurrentState](Docs/CurrentState.md) for behavior
and limits, and the [Candidate03 pass record](Docs/Passes/Candidate03.md) for
separate implementation, test, visual and audio evidence.

The published playable build remains the
[Candidate02 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02).
Its [pass report](Docs/Passes/Candidate02.md) and [evidence](Evidence/Candidate02)
remain preserved. Candidate03 starts from Candidate02 source
`aa4d55d04bf375bbef41362af77eec10d9ea224f`; Candidate02's packaged gameplay
revision is `0637288d32b6fbebc67ef93d4f03e439ff38bb67`.

## Current Candidate03 controls

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Automatic carbine fire; shotgun one discharge per press |
| R | Manual reload; fire input can interrupt shotgun shell loading |
| Left Shift | Sprint and interrupt reload; held Shift takes priority over reload |
| 1 / 2 | Select 5.56 mm Carbine / 12-Gauge Pump Shotgun |
| Tab / either mouse-wheel direction | Cycle carried weapons |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or after death |
| F1 | Enter or leave developer sandbox; resets the encounter |
| F2 / F3 | In sandbox: spawn one / up to six registered infected |
| F4 | In sandbox: refill both weapons and clear unfinished weapon operations |
| F5 | In sandbox: reset the encounter |
| F6 | In sandbox: clear corpses, detached parts, spent cases and blood |
| F7 | In sandbox: toggle bright/dim room lighting |

Player walk/sprint speeds are 225/370 cm/s; infected pursuit is 195 cm/s.
An empty equipped weapon automatically reloads when ready and reserve is
available. Held Shift defers both manual and automatic reload, even while
stationary. Interrupted reloads retain earned ammunition; switching retains
both slots' ammunition and the shotgun's pending pump/ejection obligation.

The carbine starts with 24 loaded rounds and 192 reserve; the shotgun has six
shells and 36 reserve. Normal rounds begin after five seconds. Later rounds add
48 reserve carbine rounds and eight shotgun shells, within their caps. There is
one infected archetype, 100 points per registered kill and at most 18 active
enemies. Sandbox disables automatic waves.

Pellets retain separate body/head/left-arm/right-arm/left-leg/right-leg damage
and spatial data before one transaction per victim. A blast can sever multiple
eligible parts; head loss, left-leg loss or losing both arms is fatal. Death and
detached parts start from the evaluated pose with real skeletal physics. Finite
wounds produce surface-projected trails and growing pools. Supported settled
remains can retain that pose as collidable kinematic bodies; fresh corpse hits
resume physics. This latest rest behavior is still under rendered validation.

## Play or build

For the published Candidate02 build, download its complete Windows archive,
extract it and launch the root `ProjectONE.exe`. Keep all extracted files
together. Candidate01 and Candidate02 remain separate rollback candidates.
The controls and behavior above describe Candidate03 source, not that older
release.

Git LFS is required for the actual Unreal Content, editable Blender sources and
original audio. Retrieve the repository into a writable directory outside
synchronized folders:

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git lfs pull
git lfs fsck
```

Select the intended source revision before building; cloning does not establish
that a development checkpoint is a verified release. The active development
branch is `codex/candidate03`. Its final package source revision and release
link are pending and will be recorded in the Candidate03 pass report.

For a Candidate03 source checkout, install Unreal Engine 5.7.2 and Visual Studio
C++ build tools with a compatible Windows SDK, then run:

```powershell
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1
.\Scripts\Validate-Packaged.ps1 -Candidate Candidate03
.\Scripts\Launch.ps1 -Candidate Candidate03
```

`Build.ps1` defaults to the editor target. Build/package scripts accept
`-EngineRoot` or `UE_ROOT`, with the standard Epic Games UE_5.7 installation as
fallback. `Package.ps1` builds/cooks the complete Windows Development package
at `Packaged/Candidate03/Windows`; it does not rebuild Candidate02. Launch and
validation default to Candidate03. `Launch.ps1 -Sandbox` opens its developer
sandbox; `Launch.ps1 -Candidate Candidate02` launches a retained local Candidate02
package if present. No validation flags are needed for ordinary play.

The default Candidate03 validation suite runs 13 separate modes. Optional dense
capture and CSV profiling runs are separate; numerical checks and frame times do
not establish visual or audio approval. The final fresh-checkout build result,
source hashes, archive checksum and extracted-package review are pending.

The development toolchain is UE 5.7.2, MSVC 14.50.35725 and Windows SDK
10.0.26100.0; Unreal warns that this compiler is newer than its preferred 14.44
release. Blender 5.1.2 is needed only for art authoring/regeneration. Players do
not need Blender or the Unreal Editor. Engine/compiler installation is separate
from the repository.

## Editable sources and evidence

- `Source/ProjectONE`: gameplay, weapon definitions, native animation and opt-in checks.
- `Content/ONE`: imported runtime assets and the existing Containment map.
- [Candidate03 characters](ArtSource/Characters/Candidate03/README.md): directional locomotion, turns and modular infected sources.
- [Weapon presentation inventory](ArtSource/Weapons/Candidate03/presentation_inventory.json): original brass/flash source mappings; [Candidate02 weapon sources](ArtSource/Weapons/Candidate02/README.md) retain the two authored weapons and reload actions.
- [Candidate03 audio](ArtSource/Audio/Candidate03/README.md): six original shot profiles per weapon, selected without immediate repeats; existing separate operation events remain in use.
- `Scripts`: generation, import, metadata sanitation, source-hash auditing, capture, validation and packaging. Unreal audio import commandlets require `-AllowCommandletAudio`.
- [Candidate03 evidence](Evidence/Candidate03): internal stage records. Earlier failed captures and corrected runs are distinguished in the pass report; none is a final release claim.

The [provenance](Docs/Provenance.md), [character pipeline](Docs/CharacterPipeline.md)
and [environment pipeline](Docs/EnvironmentPipeline.md) preserve the authoring
context. Characters remain simplified and original synthesized audio has not
received perceptual approval. No external sound pack or Project Zero content was
used. No broad license has been selected for the original game code or art.
