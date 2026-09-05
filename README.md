# Project ONE — Candidate05

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate05 improves close-cursor aim, responsive firing, committed magazine
reloads, live-hit feedback, locomotion, three infected attack families, the HUD
and menus, environmental audio, and carried upgrade effects.

This source checkpoint is under validation. The preserved
[Candidate04 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate04)
remains the last verified public package until Candidate05 publication completes.
See [CurrentState](Docs/CurrentState.md), the [pass report](Docs/Passes/Candidate05.md)
and [evidence index](Evidence/Candidate05/README.md) for the actual status.

## Controls

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Hold an eligible press for M4A1/Overcurrent; one eligible press per pistol/shotgun shot |
| R | Reload; shotgun loads individual shells |
| Left Shift | Sprint; magazine reload continues |
| 1 / 2 | Select an available owned slot |
| Tab / mouse wheel | Cycle available owned weapons |
| Hold F | Focused machine action; hold 0.4 seconds, release before the next action |
| H | Help and sandbox tool tray |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or dead; mouse buttons also available |
| F1 | Enter/leave sandbox and reset the encounter |
| F2 / F3 | Sandbox: spawn one / up to six infected |
| F4 | Sandbox: refill available weapons; committed reloads and reserved instances remain protected |
| F5 / F6 | Sandbox: reset / clear remains, blood, cases and magazines |
| F7 | Sandbox: bright/dim lighting |
| T | Sandbox: add 10,000 test points |
| Z / X / C / V | Sandbox: next box pistol / M4A1 / 870 / random |

Rejected firing presses do not wait for a later ready state. Release ends an
automatic burst; reload, equip, handoff, pause and input flush require a fresh
release/press. M4A1 is configured for 0.100 seconds per shot (600 RPM), Overcurrent
for 0.0869565 seconds (690 RPM). [Measured cadence](Evidence/Candidate05/WeaponTiming.md)
is reported separately.

**Magazine reloads are committed.** Shift, fire, repeated R, slot selection,
inventory acquisition and machine transfer cannot cancel them or queue a switch.
Movement remains available. Request a switch again after completion. Shotgun
loading can stop with an earned shell; allow the hand/mechanism to return safely,
then make a fresh eligible firing press. Empty equipped weapons automatically
reload from reserve. Deliberate empty clicks sound only when both pools are zero.
These rules replace the previous sprint-cancel and deferred-shot behavior.

Normal play starts with M1911 7/56 and an empty second slot. Exactly two slots,
100-point registered kills, the 950-point/five-second Mystery Box and
5,000-point/nine-second Pack-a-Punch remain. Ready rewards wait at their physical
machine for deliberate collection. Pack-a-Punch reserves and returns the same
instance and slot; depositing the only weapon leaves the player unarmed.
There are six definitions, three families and one upgrade tier.

## Build and run

Install Git LFS, Unreal Engine 5.7.2, compatible Visual Studio C++ tools and a
Windows SDK. Use a writable checkout outside synchronized folders.

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git checkout codex/candidate05
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate05
.\Scripts\Validate-Packaged.ps1 -Candidate Candidate05
.\Scripts\Launch.ps1 -Candidate Candidate05
```

Candidate05 is the script default. Its output is `Packaged/Candidate05/Windows`;
earlier candidate packages remain separate. Add `-Sandbox` to Launch for developer
tools. Extracted releases run their root `ProjectONE.exe`, keeping the complete
folder together. Packaging includes required Windows prerequisites and notices.
Dense recording and recording-free profiling are separate validation passes.

## Editable sources

The [combat rules](Docs/CombatRules.md), [motion](ArtSource/Characters/C05/README.md),
[HUD](ArtSource/UI/Candidate05/README.md), [audio](ArtSource/Audio/Candidate05/README.md),
[weapon workshop](ArtSource/Weapons/Candidate04/README.md) and
[provenance](Docs/Provenance.md) retain original assets and repeatable importers.
Blender 5.1.2 is used for source regeneration. No external font, asset pack, paid
service, new broad license or Project Zero content is introduced.

Implementation, automated input dispatch, native desktop input, moving visual
review, auditory review and package verification are separate claims. Technical
checks do not establish user approval of visual direction or sound quality.
