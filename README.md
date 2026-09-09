# Project ONE — Candidate05

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate05 adds responsive firing, committed magazine reloads, stronger hit
feedback, revised movement and attacks, a compact HUD, industrial audio and
persistent upgrade effects.

**Local package verified:** 17 engine automation tests, 19 default packaged modes
/ 3,942 assertions, and 10 supplemental runs / 3,506 assertions pass. Four
recording-free rifle/crowd profiles and both movie encode/full-decode checks
are complete. Native desktop checks cover 13 limited discrete actions.

[Windows package](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/ProjectONE-Candidate05-Windows.zip) · [Motion film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/motion_review.mp4) · [Presentation film](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/presentation_review.mp4) · [Prerelease page](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate05)

Extract the whole release folder and run its root `ProjectONE.exe`. Keep its
runtime files, prerequisites and notices together.

Publication is verified only by the [Candidate05-PublicationVerification.json](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate05/Candidate05-PublicationVerification.json) attachment, issued after actual public downloads and Git LFS checks. It identifies the reviewed [candidate05 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate05), documentation revision D, packaged source S and downloaded asset hashes. These pages report locally verified evidence until that attachment is issued.

Packaged source S is `6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Its clean public-source
Editor/Game/package build and 45-file runtime audit pass. Five warnings remain
from three historical engine fixtures. The ZIP and local extraction pass
hash/CRC checks. See the [current state](Docs/CurrentState.md),
[pass report](Docs/Passes/Candidate05.md) and
[evidence index](Evidence/Candidate05/README.md) for source-bound results.

The [native-control retry](Evidence/Candidate05/Verification/native_input.json)
verified sandbox, one shot, reload, tools, pause/resume, restart and quit after
the user handled the initial Windows Security prompt. The 13 actions include
a brief W tap that did not establish movement; held movement and machine
interaction were not tested.

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

Rejected presses do not wait for the gun to become ready. Release ends an
automatic burst; reload, equip, handoff, pause and input flush require a fresh
release/press. **Magazine reloads are committed:** Shift, fire, repeated R,
switching and machine transfer cannot cancel them or queue a later switch.
Movement remains available. Request the switch again after completion.
Shotgun loading retains earned shells, then requires safe hand/mechanism return
and a fresh eligible firing press. Empty equipped weapons reload from reserve;
deliberate dry clicks require both ammo pools to be zero.

M4A1 is configured for 0.100 seconds / 600 RPM; Overcurrent for 0.0869565 seconds /
690 RPM. The cadence change raises their ideal burst DPS by 60%, from 200/460
to 320/736, with damage unchanged. Final packaged full-magazine intervals measure
M4A1 at 599.99–600.00 RPM and Overcurrent at 684.78/688.52/688.51 RPM under
30/60/120 caps; [timing evidence](Evidence/Candidate05/Verification/weapon_timing.json)
retains frame quantization and finite-window limits. These are discharge rates,
not gameplay FPS or native-input latency.
Normal play still starts with M1911 7/56 and an empty second slot. Two owned
slots, 100-point registered kills, the 950-point/five-second Box and
5,000-point/nine-second Pack-a-Punch remain. Ready rewards wait at the physical
machine; upgrades reserve and return the same weapon instance and slot.

## Build and run

Install Git LFS, Unreal Engine 5.7.2, compatible Visual Studio C++ tools and a
Windows SDK. Use a writable checkout outside synchronized folders.

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git checkout 6b8621cef9d2a87de6e6eabc1743359c6274da5a
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate05
.\Scripts\Validate-Packaged.ps1 -Candidate Candidate05
.\Scripts\Launch.ps1 -Candidate Candidate05
```

For reproducibility, check out the reported packaged source S before building.
Output is `Packaged/Candidate05/Windows`; older candidates remain separate.
Add `-Sandbox` to Launch for developer tools. Offscreen validation can use
`-RenderOffscreen`, which forces the requested viewport size. Recording and
recording-free profiling are separate passes.

## Sources and review limits

[Combat rules](Docs/CombatRules.md), [motion](ArtSource/Characters/C05/README.md),
[HUD](ArtSource/UI/Candidate05/README.md), [audio](ArtSource/Audio/Candidate05/README.md),
[weapon sources](ArtSource/Weapons/Candidate04/README.md) and
[provenance](Docs/Provenance.md) retain the original editable assets and importers.
The pass report consolidates failed build/fixture checkpoints. No broad license,
external asset pack, paid service or Project Zero content is introduced.

All 24 [UI stills across four sizes](Evidence/Candidate05/UI/four_sizes_review.md)
and bounded [final chronological images](Evidence/Candidate05/Visual/README.md)
were inspected. The [movies](Evidence/Candidate05/Media/README.md) contain actual
engine imagery/audio and pass full decode; their short video/audio endpoint
differences are disclosed. The [four profiles](Evidence/Candidate05/Performance/FinalS4/README.md)
retain all frames and spikes; their 120 FPS cap is not a claim of locked 120 FPS. Brief reel crossfade, mechanical motion and subtle hand/effect
details remain documented; this is not continuous film playback.
Automated checks, native desktop input and audio listening are separate evidence.
Both listening attempts, including September 9, confirmed unavailable audio
input. The final captures retain the actual engine mix. Their
[numerical audio review](Evidence/Candidate05/Audio/README.md) reports no full-scale
samples, while preserving the presentation phase-energy FAIL for silent pause
phases 92/93 and death phase 96. Timbre/localization approval remains unverified.
Technical completion does not imply user approval of game feel or visual/audio
direction. The one proposed next milestone is a focused human playtest and polish
pass on movement, attack feel and the audio mix.
