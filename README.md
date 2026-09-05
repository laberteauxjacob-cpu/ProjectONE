# Project ONE — Candidate04

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate04 adds a playable weapon progression loop to the existing arena:
M1911 start, Mystery Box acquisition, physical Pack-a-Punch deposit, reserved
inventory slot, nine-second processing and deliberate upgraded-weapon retrieval.

The verified gameplay source is
[`8055041ebc98a4df7cd8923b05e7b89ad7372e38`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/8055041ebc98a4df7cd8923b05e7b89ad7372e38).
A clean public checkout built the Editor, Win64 game and Development package.
All nine native tests and all 15 packaged modes passed, with 984 packaged
assertions and zero failures. Three native tests retain five isolated test-world
teardown warnings; the four inventory tests are warning-free.

The complete Windows archive is audited and was extracted into a separate
Candidate04 package. A packaged walkthrough passed 58 checks and produced 3,001
original frames with genuine engine audio. Bounded visual inspection found
readable machine actions and case flight, with fine hand/mechanism contact and
intermittent missing HUD glyphs still unresolved. Ordinary sandbox launch was
observed, but a Windows Security prompt blocked native gameplay input; no
native held-input or auditory approval is claimed.

The [full progression movie](Evidence/Candidate04/progression_loop.mp4) is
148.117 seconds with 80 verified chapter titles; its
[sidecar](Evidence/Candidate04/progression_loop.json) records the one-frame idle
omission and container-only chapter repair. The
[67.9-second weapon comparison](Evidence/Candidate04/weapon_comparison.mp4)
uses eight excerpts; its [sidecar](Evidence/Candidate04/weapon_comparison.json)
records the source intervals and final encoding identity.

[Three media-free profiles](Evidence/Candidate04/Performance/README.md) at
1600x900, cap 120 and VSync off measured full mean frame times of
8.335–8.412 ms and p99 of 9.361–9.709 ms. They include moving Last Word combat
and operating machines; requested live counts are targets, not constant counts.
One run per count and the different Candidate03 workload preclude a causal
optimization or GPU-cost claim.
The
[candidate04 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate04)
carries the Windows archive and evidence. Its separate
[Candidate04-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate04/Candidate04-PublicationVerification.json)
attachment records verification performed after upload: exact final commit,
downloaded hashes, full outgoing history audit and hydrated public checkout/LFS.
The built gameplay source above remains distinct from that release revision.
The [Candidate03 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
remains the playable rollback, with its tag and archive
[verified unchanged](Evidence/Candidate04/Verification/preservation.json).

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

The machines show those actions physically: the containment chest unlocks and
opens on hydraulic struts, while the processor draws the actual weapon assembly
into a moving cradle, closes opposing clamps and shields, then returns the
upgraded assembly. Moving field rings, state lighting and original mechanical
audio accompany the cycle.

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
git checkout 8055041ebc98a4df7cd8923b05e7b89ad7372e38
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
The source above is the exact verified build revision; later documentation and
evidence commits need not change its runtime identity. Fresh-checkout verification
used an existing development host, not a clean Windows installation.

The audited `ProjectONE-Candidate04-Windows.zip` contains 1,217 entries and is
407,635,789 bytes. Its SHA-256 is
`1ad1a3258f16fccf12451fff829aa9124321491a457232b88efa871826ba783c`.
See the [archive verification](Evidence/Candidate04/Verification/archive.json).
Source is on [codex/candidate04](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/codex/candidate04);
the release review uses [candidate04](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate04)
and the [Candidate03 comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate03...candidate04).

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
The public evidence tools adapt only idle-tail assembly and container chapter
metadata. They do not rebuild or modify the S2 game; stream-copy chapter repair
preserves the encoded audio/video payloads.

The next focused pass should polish weapon handling, machine contact and HUD
readability at the normal camera distance, with actual held-input playtesting
and auditory review before adding content or systems.
