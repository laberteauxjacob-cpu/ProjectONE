# Project ONE — Candidate04

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate04 adds a playable weapon progression loop to the existing arena:
M1911 start, Mystery Box acquisition, physical Pack-a-Punch deposit, reserved
inventory slot, nine-second processing and deliberate upgraded-weapon retrieval.

This is the implementation checkpoint. Final public-source rebuilding,
packaged verification and Candidate04 release publication are still in progress.
The [Candidate03 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
remains the verified playable rollback, with its tag and archive preserved.

See [CurrentState](Docs/CurrentState.md), the
[Candidate04 pass report](Docs/Passes/Candidate04.md) and
[evidence index](Evidence/Candidate04/README.md) for exact behavior and evidence.

## Controls

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Automatic M4A1/Overcurrent; other families fire once per press |
| R | Reload; shotgun loads individual shells |
| Left Shift | Sprint and cancel unfinished reload events |
| 1 / 2 | Select an available owned slot |
| Tab / mouse wheel | Cycle available owned weapons |
| Hold F | Focused machine action; hold for 0.4 seconds, release before the next action |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or dead |
| F1 | Enter/leave sandbox and reset the encounter |
| F2 / F3 | Sandbox: spawn one / up to six infected |
| F4 | Sandbox: refill available weapons; reserved instances stay untouched |
| F5 / F6 | Sandbox: reset / clean remains, blood, cases and dropped magazines |
| F7 | Sandbox: bright/dim lighting |
| T | Sandbox: add 10,000 clearly labelled test points |
| Z / X / C / V | Sandbox: next box pistol / M4A1 / 870 / random |

Normal rounds start with an M1911, 7 loaded rounds, 56 reserve and an empty
second slot. Registered kills award 100 points. The Mystery Box costs 950 and
cycles actual weapon models for five seconds. An empty slot fills first;
otherwise collection previews replacement of the selected available weapon.
An already-owned family refills its available effective variant, including an
upgrade. The paid result waits indefinitely for a fresh deliberate collection.

Pack-a-Punch costs 5,000 at physical acceptance. The same weapon instance and
slot remain reserved through the nine-second process. Another available weapon
can be used while waiting; depositing the only weapon leaves the player unarmed.
The output waits indefinitely. Fresh Hold F retrieves and refills that exact
instance as Last Word, Overcurrent or Gravebreaker. There is one upgrade tier.
See the pass report for capacities, cadence, bounded penetration and recovery.

Pistol and rifle reloads release a separate old magazine with gravity, bounce,
settling and an eight-second lifetime, capped at twelve. Cancelled reloads retain
earned events; if a loaded weapon's magazine has been removed, press R to finish
reloading before firing. The 870 retains its tube reload and pump obligation.
Candidate03 movement, regional blood, remains and the existing arena are retained.

## Build and run

Git LFS is required for Unreal assets, editable Blender/FBX sources and audio.
Use a writable checkout outside synchronized folders. Install Unreal Engine
5.7.2, compatible Visual Studio C++ tools and a Windows SDK separately.

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git checkout codex/candidate04
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate04
.\Scripts\Validate-Packaged.ps1 -Candidate Candidate04
.\Scripts\Launch.ps1 -Candidate Candidate04
```

Packaging writes `Packaged/Candidate04/Windows`; it preserves Candidate03.
Launch, package and validation default to Candidate04. `Launch.ps1 -Sandbox`
opens its developer sandbox. Extracted releases run their root `ProjectONE.exe`
with all files kept together; Windows prerequisites are included when packaged.
Dense recording and CSV profiles are separate from the default validation suite.

## Editable sources and evidence limits

The [weapon workshop](ArtSource/Weapons/Candidate04/README.md),
[machines](ArtSource/Machines/Candidate04/README.md) and
[audio sources](ArtSource/Audio/Candidate04/README.md) retain original editable
assets and generation scripts. Blender 5.1.2 is needed for art regeneration.
[Provenance](Docs/Provenance.md) records the retained Candidate03 foundation.
No Project Zero content, external asset pack, paid service or broad license was
introduced. Prior tags, releases and local packages remain separate.

Production-input automation, native desktop input, chronological frame review,
engine audio measurements and performance are reported separately. Available
native tooling cannot establish sustained held-key playtesting or perceptual
listening. Character fingers remain fixed geometry; small props and hand contacts
remain subject to camera scale and occlusion. The user's visual approval remains
separate from technical verification.
