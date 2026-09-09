# Project ONE — Candidate06

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate06 adds target-independent aiming, weapon spread and body penetration,
headshot damage and combat rewards, player recovery, three pickups, and more
usable Mystery Box and Pack-a-Punch interactions.

Candidate06 is in verification. Its development Editor build, 28 automation
tests and recorded six-weapon combat check pass. Packaging, clean public-source
verification and publication are pending. The latest verified public playable
release remains [Candidate05](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate05).
See the [current state](Docs/CurrentState.md) and [Candidate06 pass](Docs/Passes/Candidate06.md)
for current evidence and limitations. Earlier candidate tags and releases remain intact.

## Controls

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Fire; hold for M4A1/Overcurrent, one eligible press per pistol/shotgun shot |
| Hold right mouse + left mouse | Shoot at head height |
| Hold Left Ctrl + left mouse | Shoot low; takes priority if both height modifiers are held |
| No height modifier | Torso-height aim |
| R | Reload; shotgun loads individual shells |
| Left Shift | Sprint; ordinary magazine reload continues |
| 1 / 2 | Select an available owned slot |
| Tab / mouse wheel | Cycle available owned weapons |
| Hold F at the Box | Purchase, then a separate hold to collect |
| Tap F at Pack-a-Punch | Deposit the equipped base weapon; no reload/ready requirement |
| Approach a ready Pack-a-Punch | Automatic return; no F required |
| H | Help and sandbox tools, forced pickups and shake strength |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or dead; mouse buttons also available |
| F1 | Enter/leave sandbox and reset the encounter |
| F2 / F3 | Sandbox: spawn one / up to six infected |
| F4 | Sandbox: refill available weapons |
| F5 / F6 / F7 | Sandbox: reset / clear remains / bright or dim lighting |
| T | Sandbox: add 10,000 test points |
| Z / X / C / V | Sandbox: next eligible Box pistol / M4A1 / 870 / random |

**Only left mouse fires.** Holding a height modifier alone never fires. Releasing
both modifiers restores torso height. The wheel changes weapons, not aim height.

Rejected firing presses do not queue for later. Releasing left mouse ends an
automatic burst. Ordinary magazine reloads remain committed through sprint,
fire and weapon-switch requests. Pack-a-Punch transfer may reserve a gun during
reload or pumping, preserving earned events and protecting the other weapon.

Normal play starts with M1911 7/56 and an empty second slot. The Box costs 950,
reveals after five seconds and excludes families already owned or reserved.
Pack-a-Punch costs 5,000 and takes nine seconds. Approach its front within reach
for automatic return to the same slot. **A ready upgrade has a 15-second
retrieval deadline, then is permanently lost without a refund.**

Each distinct living victim damaged by one discharge earns 10 impact points,
plus 100 for a body kill or 120 for a qualified head kill. One shell awards
impact points per victim, not per pellet. Head health damage uses 1.5×; cosmetic
head trauma cannot execute a healthy target by itself. After 15 damage-free
seconds, health recovers at 10% of current maximum per second. Red edges track
missing health.

Natural drops use one 1% total chance per registered death with equal initial
type weights. Insta-Kill and Double Points last 30 seconds; recollection refreshes
the timer. Max Ammo refills available owned guns. Uncollected pickups last
30 seconds. These are provisional gameplay tuning values. See the complete
[combat rules](Docs/CombatRules.md) and [survival/rewards tuning](Docs/Candidate06SurvivalRewards.md).

## Build and run

Install Git LFS, Unreal Engine 5.7.2, compatible Visual Studio C++ tools and a
Windows SDK. Use a writable checkout outside synchronized folders.

```powershell
git lfs install
git clone --branch codex/candidate06 https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate06
.\Scripts\Launch.ps1 -Candidate Candidate06
```

For release reproduction, check out the packaged source revision reported in
the completed pass before building. Output is `Packaged/Candidate06/Windows`.
Extract the whole release folder and run its root `ProjectONE.exe`; keep runtime
files, prerequisites and notices together. Add `-Sandbox` to Launch for tools.
`Validate-Packaged.ps1` runs numeric fixtures; the separate Portability06 mode
requires `-Modes ONE06PortabilityCheck -Map /Game/ONE/Maps/Portability06`.

## Sources and review limits

[Provenance](Docs/Provenance.md), [pickup sources](ArtSource/Pickups/Candidate06/README.md),
[pickup audio](ArtSource/Audio/Candidate06/README.md), [existing motion](ArtSource/Characters/C05/README.md)
and [weapon sources](ArtSource/Weapons/Candidate04/README.md) identify editable
assets and importers. The new pickup cues are designed synthetic sounds, not
recorded physical effects. No Project Zero content, external asset pack,
paid service or broad licensing change is introduced.

Implementation, numerical checks, in-engine recordings, visual inspection,
audio listening and native desktop input are separate evidence. Passing one
does not establish the others or user approval of game feel. The next proposed
milestone is the [infected visual/physical overhaul and recorded-audio replacement](Docs/Roadmap.md).
Perk chambers remain a later milestone.
