# Project ONE — Candidate07

Candidate07 gives the infected three distinct appearances, connected attacks,
physical contact and recoverable falls, with recorded gun reports, voices and
Foley. Maintenance, Laboratory and Facility Staff share one gameplay archetype;
their clothing, faces and hair differ. The player, six weapon variants, two-slot
inventory, machines, survival and reward rules retain the Candidate06 foundation.

**Release status: Windows prerelease prepared; native control review is blocked and visual/audio approval remains provisional.** Gameplay source S:
`3d75c2faa0cecef6075f00755d8e4f5dd2562817`. Publication revision D: [the candidate07 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate07).
Windows package: [ProjectONE-Candidate07-Windows.zip](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/ProjectONE-Candidate07-Windows.zip), SHA-256 `e3dae5b6ead801d03c6bc67362f8849ae465df88bd1e691ceed2681b18e61348`.
The [publication-verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/Candidate07-PublicationVerification.json) identifies actual D and records public refs, anonymous downloads, fresh D checkout/LFS and prior-candidate preservation when issued. These later public checks are not asserted by this committed text.

The fresh public S checkout built Editor, Game and the Windows package and passed all 39 engine tests. All 26 C07 packaged runs passed 7,506 driver/companion assertions; six separate C06 baseline profiles passed 228. Four final engine recordings passed full decoding and numerical PCM checks. Native desktop capture was blocked; no native input pass or perceptual audio audition is claimed.

The [pass report](Docs/Passes/Candidate07.md) separates build and packaged checks,
actual frame review, numerical audio checks, native input and performance.
Technical verification does not establish user approval of appearance or feel.
**No perceptual audio audition is claimed.**

## What changes in play

- Hanging arms, full-body turns and five sided performances across three attack
  families replace the old repeated attack sequence. Selection accounts for
  limbs, approach and bearing; a committed swing still has one damage contact.
- Upper bodies yield to contact. A fallen infected stays alive and damageable,
  retains missing limbs and gets up only when local support and clearance allow.
  Recovery around other bodies uses bounded physical effort; walls and crowded
  floor space can still delay it.
- Recorded physical shots, reload mechanisms, footsteps, creature performances
  and impacts follow gameplay events. Intentional upgrade energy layers and the
  existing machine/pickup accents remain. Recorded analogues and source rights
  are identified in the [audio source notes](ArtSource/Audio/Candidate07/README.md).

## Controls

| Action | Input |
| --- | --- |
| Move / aim | WASD / mouse, independently |
| Fire | LMB; hold for M4A1/Overcurrent, one eligible press for pistol/shotgun |
| Head / low / torso height | Hold RMB / hold Left Ctrl / release both; LMB still fires, Ctrl takes priority |
| Reload / sprint | R / Left Shift |
| Select / cycle available guns | 1 or 2 / Tab or mouse wheel |
| Mystery Box | Hold F to buy; hold F separately to collect |
| Pack-a-Punch | Tap F to deposit; approach the ready gun for automatic same-slot return |
| Help and tools / pause | H / Escape |
| Restart / quit when paused or dead | Enter / Q |

Sandbox tools: F1 toggles sandbox and restarts the scene; F2/F3 spawn one/six,
F4 refills, F5 resets, F6 clears remains, F7 switches dim/bright lighting, and
T grants points. Z/X/C/V choose forced pistol/carbine/shotgun/random Box results.
H includes the three forced pickups and shake strength.

Low aim is a fixed plane 65 cm above the player's floor. A fully flattened body
can lie below it: real Ctrl+LMB shots reproduced this miss. The game does not
snap aim to bodies or enlarge regions to conceal it. This is a known targeting
limitation, detailed in [CurrentState](Docs/CurrentState.md) with its
[retained failed check](Evidence/Candidate07/Verification/low_aim_compatibility.md).

## Play and build

Extract the complete Windows release and run its root `ProjectONE.exe`, keeping
runtime files, prerequisites and notices together. Native launch and input scope:
BLOCKED_NATIVE_CAPTURE: Windows returned cursor-access and monitor-capture errors after a fresh-window retry. Zero native actions or usable images; only the launched test process was terminated, and all six runtime files still match. Normal Quit and held-input review were not verified.

For source builds, use Git LFS, Unreal Engine 5.7.2, compatible Visual Studio C++
tools and a Windows SDK, in a writable checkout outside synchronized folders:

```powershell
git lfs install
git clone --branch codex/candidate07 https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git lfs pull
git lfs fsck
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate07
.\Scripts\Launch.ps1 -Candidate Candidate07
```

Configure the installed engine as described by the build scripts. Output is
`Packaged/Candidate07/Windows`; add `-Sandbox` to Launch for developer tools.
Build the verified Candidate07 publication tag or branch at D. D preserves S's
gameplay and assets and adds documentation, evidence and scoped transport
attributes for notices, processing tools and inventory/recipe files. Keep those
attributes when checking out source; detaching the earlier S would lose them.
The supplied playable binary remains the fresh package built from S.
Historical [Candidate06](Docs/Passes/Candidate06.md) remains preserved.

[Current rules](Docs/CombatRules.md), [infected contract](Docs/Candidate07Infected.md),
[editable art and import order](ArtSource/README.md) and
[provenance](Docs/Provenance.md) document the implementation and sources.
No broad project license is assigned. The next focused recommendation is
floor-level mouse targeting; [perk chambers and future map entrances](Docs/Roadmap.md)
remain separate, unstarted work.
