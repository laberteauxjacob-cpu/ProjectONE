# Project ONE — Containment candidate 01

An original solo, top-down facility survival scene built with Unreal Engine 5.7.2. This is a visual candidate for review. Character anatomy, cloth, foot planting, turns and audio still need focused polish before this can be considered convincing production art.

## Play

Double-click `Packaged/Candidate01/Windows/ProjectONE.exe`, or run `Scripts/Launch.ps1` from PowerShell. The launch script opens the same game at 1600×900, windowed. No character-selection or candidate flags are required. Keep the entire `Windows` folder together; the root executable is its launcher.

| Control | Action |
| --- | --- |
| WASD | Move |
| Mouse | Aim independently of movement |
| Left mouse | Fire; hold for automatic fire |
| R | Reload |
| Shift | Run |
| Escape | Pause / resume |
| Enter | Restart while paused or after death |
| Q | Quit while paused or after death |

The first round begins after five seconds. Survive successive rounds, earn 100 points per kill, and receive 48 reserve rounds between waves. The carbine holds 24 rounds. Head hits remove the head and kill; two hits to the removable arm sever it while the infected can continue attacking. Aim at visible body regions, not just the ground beneath them. Maximum active enemies: 18.

## Source and rebuild

- Source project: `ProjectONE.uproject`; C++ responsibilities are separated under `Source/ProjectONE`.
- Editable Blender rigs, meshes and actions: `ArtSource/Characters/{CharacterWorkshop,Response,Infected}.blend`.
- Editable facility kit and carbine: `ArtSource/Environment/ProjectONE_IndustrialKit.blend`.
- FBX exports, source audio and blood mask: `ArtSource/Exports`, `ArtSource/Audio`, `ArtSource/Textures`.
- Generation, import, audit, build and test scripts: `Scripts`.

From this project directory in PowerShell:

```powershell
& .\Scripts\Build.ps1
& .\Scripts\Package.ps1
& .\Scripts\Validate-Packaged.ps1
& .\Scripts\Launch.ps1
```

Build scripts use `UE_ROOT` or their `-EngineRoot` argument, falling back to the standard Epic Games UE_5.7 installation under Program Files. Verified tools: UE 5.7.2, Blender 5.1.2, VS Build Tools 2026 / MSVC 14.50.35725, Windows SDK 10.0.26100.0. UE reports this compiler as newer than its preferred version; actual editor and game builds passed. Blender is needed only to edit/regenerate assets. Playing the packaged executable needs neither Blender nor the Unreal Editor nor Project Zero.

Clone this project into a writable directory outside synchronized folders. Project Zero was left untouched; no old assets were copied or referenced by the new game.

## Public repository

Start with [Docs/CurrentState.md](Docs/CurrentState.md). Git LFS is required for Unreal assets, Blender files, FBX files, source WAVs and recordings. After cloning, run `git lfs install` and `git lfs pull`. The full original `Content` and `ArtSource` trees are included. Install Unreal Engine and build tools separately; no engine installation files are stored here. No project license has been selected; do not add a broad code or art license without the owner's approval.

The public Candidate01 tag is a sanitized baseline snapshot. Its gameplay code matches the preserved original local Candidate01, but private build logs, local paths and unpublished author metadata were excluded. The original local history and package remain available locally. Subsequent public candidates use ordinary commits and tags descending from this baseline.

## Review evidence

See [validation and remaining work](Docs/Validation.md), [asset provenance](Docs/Provenance.md), [visual direction](Docs/Direction.md), [character pipeline](Docs/CharacterPipeline.md), and [environment pipeline](Docs/EnvironmentPipeline.md).

`Evidence/Packaged` contains final standalone reports and genuine engine screenshots. `Evidence/ScriptedGameplay.gif` is a silent, timestamped, low-frame-rate capture from a scripted gameplay run using the normal gameplay camera. It is not a manual playthrough or a performance recording. Raw PNGs remain under the packaged game's `ProjectONE/Saved/Presentation` directory.

Recommended next focused milestone: review this candidate, then refine the chosen character shapes, hand/weapon contact, foot planting, turning and attack poses, and replace provisional synthesized audio. Keep the single room and single weapon scope until the visual direction is approved.
