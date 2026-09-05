# Project ONE — Candidate02

An original solo, top-down facility survival prototype in Unreal Engine 5.7.2. Candidate02 develops the accepted single-room foundation into a two-weapon combat sandbox. It is a playable development candidate, with provisional character polish and synthesized audio.

Start with [CurrentState](Docs/CurrentState.md) for implemented behavior and limitations, then the [Candidate02 pass report](Docs/Passes/Candidate02.md) for exact build revisions, verification and evidence. The public project includes its actual Unreal Content, editable Blender sources, original audio and asset-generation scripts.

## Play

Download the complete Windows archive from the [Candidate02 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02), extract it, and launch the root `ProjectONE.exe`. Keep all files together. A locally built package is at `Packaged/Candidate02/Windows/ProjectONE.exe`; `Scripts/Launch.ps1` launches it at 1600×900. No hidden candidate flags are required. Candidate01 remains preserved separately.

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Carbine automatic fire; shotgun one shot per press |
| R | Reload; shotgun fire input interrupts shell loading |
| 1 / 2 | Select AR-01 carbine / SG-01 pump shotgun |
| Tab / mouse wheel | Cycle carried weapons |
| Shift | Run |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or after death |
| F1 | Enter or leave developer sandbox; resets encounter |
| F2 / F3 | In sandbox: spawn one / up to six infected |
| F4 / F5 / F6 | In sandbox: refill both weapons / reset / clear remains and blood |

Normal rounds begin after five seconds. The carbine carries 24/192 rounds; the shotgun carries 6/36 shells. Each weapon retains its ammunition and pending pump state across switches. Later rounds add 48 reserve carbine rounds and eight shotgun shells, within each reserve cap. There is one infected archetype, 100 points per registered kill, and a maximum of 18 active enemies.

## Retrieve and build

Install Git and Git LFS, Unreal Engine 5.7.2, and Visual Studio C++ build tools with a compatible Windows SDK. Clone into a writable directory outside synchronized folders:

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Package.ps1
.\Scripts\Validate-Packaged.ps1
.\Scripts\Launch.ps1
```

Build scripts accept `-EngineRoot` or `UE_ROOT`, with the standard Epic Games UE_5.7 installation as fallback. `Build.ps1` builds the editor target; add `-Game` for the game target. `Package.ps1` builds and cooks the complete Windows Development package. The validation script opens separate game runs, including genuine capture and independent performance runs; allow several minutes.

The verified local toolchain is UE 5.7.2, MSVC 14.50.35725 and Windows SDK 10.0.26100.0. Unreal warns that this compiler is newer than its preferred 14.44 release. See the pass report for the actual fresh-checkout build result. Blender 5.1.2 is needed only to edit or regenerate art; players do not need Blender or the Unreal Editor. The engine and compiler are installed separately, not redistributed as source dependencies.

The comparison run alone overrides Unreal's background mute so recording can continue while another window has focus. To check its WAV, use Python 3 with `Scripts/audit_gameplay_audio.py --source Packaged/Candidate02/Windows/ProjectONE/Saved/Candidate02/Comparison/gameplay_master.wav --require-phase-coverage --require-audible-phases`. `Scripts/assemble_candidate02_capture.py` accepts that comparison folder with `--input`, an MP4 path with `--output`, and an installed FFmpeg executable with `--ffmpeg`. Ordinary gameplay does not require these recording options or tools.

## Editable sources and evidence

- `Source/ProjectONE`: native gameplay, weapon definitions, animation blending and opt-in checks.
- `Content/ONE`: complete imported runtime assets and the existing Containment map.
- `ArtSource/Characters` and `ArtSource/Environment`: accepted character rigs and facility kit.
- `ArtSource/Weapons/Candidate02`: editable weapon/animation workshops, source previews and [readable asset inventory](ArtSource/Weapons/Candidate02/inventory.json).
- `ArtSource/Exports`, `ArtSource/Audio`, `ArtSource/Textures`: original interchange meshes, clips, WAVs and textures.
- `Scripts`: generation, import, metadata sanitation, source-hash auditing, capture and build tools. Unreal audio import commandlets require `-AllowCommandletAudio`.
- [Candidate02 evidence](Evidence/Candidate02): browser-viewable screenshots, reports and genuine timestamped gameplay with engine audio. Captures and performance measurements are separate runs.

Read [Direction](Docs/Direction.md), [Provenance](Docs/Provenance.md), [CharacterPipeline](Docs/CharacterPipeline.md) and [EnvironmentPipeline](Docs/EnvironmentPipeline.md) for the existing creative and asset context. [Candidate01 validation](Docs/Validation.md) is historical evidence, not Candidate02 results.

The public `candidate01` tag is a sanitized baseline whose gameplay source matches the preserved original Candidate01. Private logs, local paths and unpublished personal author metadata were excluded before the first public push. The original local history and packaged build remain local rollback points. Subsequent public candidates descend from that baseline using ordinary commits. Project Zero remains excluded and untouched.

No broad license has been chosen for the original game code or art. Public access does not select one on the owner's behalf. No external sound pack was added; new audio is original procedural synthesis and remains subject to perceptual review.
