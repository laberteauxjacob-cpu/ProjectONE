# Project ONE — Candidate06

An original top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate06 adds target-independent aiming, consistent spread and body
penetration, headshot rewards, health recovery, three pickups, and revised
Mystery Box and Pack-a-Punch interactions.

**Release and public-download status:** [Release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate06) / [final download-verification record](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/Candidate06-PublicationVerification.json). Final verification is issued after publication; this committed text does not claim those later checks already ran.
Packaged source S3 is `c2c422012f4f219fb5e63ac24294a6ba8428f999`;
final published revision is [the `candidate06` tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate06). The clean public-source
Editor/Game/package build of S3, all 28 source tests and separate runtime audit pass.
The tests retain five warning events. These checks do not establish native
input, rendered legibility or final packaged gameplay results.
All 15 final packaged runs complete with 4,084 assertions, zero failures and
exit 0: four core capture fixtures, three weapon caps, two UI sizes and six
profile runs. Bounded still reviews cover the four captures and all 12 UI PNGs.
Measured performance and encoded media:
22,129 retained frames; means 8.3359–8.3750 ms, p99 8.9514–10.2795 ms and maxima 20.6116–31.4236 ms. Seven frames exceed 16.7 ms and none exceed 33.3 ms. One non-isolated sample per workload supplies no matched-baseline or universal-FPS claim. The four encoded recordings pass full decode and endpoint checks; see [media, audio measurements and limits](Evidence/Candidate06/Media/README.md) and [full timelines, occupancy and host limits](Evidence/Candidate06/Performance/README.md).
The [retained S1 coverage](Evidence/Candidate06/Verification/retained_coverage.md)
records 95 progression assertions and 2,708 projected-aim assertions across
480 shots on unchanged authority/driver code. Those runs were not rerun at S3.
The [limited native check](Evidence/Candidate06/Verification/native_input.json)
records 14 discrete actions and 22 input edges, including the corrected world
symbols. It does not test held firing combinations, movement feel or audio audition.
See [current state](Docs/CurrentState.md) and the [Candidate06 pass](Docs/Passes/Candidate06.md).
[Candidate05](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate05)
and earlier candidates remain preserved.

Revision D adds documentation, evidence and their attributes, plus one movie-assembly command argument in `Scripts/assemble_candidate06_capture.py`: `-vf tpad=stop_mode=clone:stop_duration=1,fps=30`. The explicit last-image extension, trimmed at the unchanged WAV endpoint, corrects the first combat encode ending about 0.259 seconds before its audio. Original failed output and diagnostics remain preserved privately. This offline assembly correction changes no gameplay source or packaged runtime; the package remains the verified S3 build without a rebuild.

## Controls

| Control | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left mouse | Fire; hold for M4A1/Overcurrent, one eligible press per pistol/shotgun shot |
| Hold right mouse + left mouse | Shoot at head height |
| Hold Left Ctrl + left mouse | Shoot low; low takes priority if both modifiers are held |
| No height modifier | Torso-height aim |
| R | Reload; shotgun loads individual shells |
| Left Shift | Sprint; ordinary magazine reload continues |
| 1 / 2 | Select an available owned slot |
| Tab / mouse wheel | Cycle available owned weapons |
| Hold F at the Box | Purchase, then a separate hold to collect |
| Tap F at Pack-a-Punch | Deposit an eligible equipped base weapon, including empty/reloading/pumping guns |
| Approach a ready Pack-a-Punch | Automatic same-slot return; no F required |
| H | Help and sandbox tools, forced pickups and shake strength |
| Escape | Pause / resume |
| Enter / Q | Restart / quit while paused or dead; mouse buttons also available |
| F1 | Enter/leave sandbox and reset the encounter |
| F2 / F3 | Sandbox: spawn one / up to six infected |
| F4 | Sandbox: refill available weapons |
| F5 / F6 / F7 | Sandbox: reset / clear remains / bright or dim lighting |
| T | Sandbox: add 10,000 test points |
| Z / X / C / V | Sandbox: next eligible Box pistol / M4A1 / 870 / random |

Only left mouse fires. Height modifiers alone never fire; releasing both
restores torso height. The wheel changes weapons. Rejected firing presses do
not queue, and releasing left mouse ends an automatic burst. Ordinary magazine
reloads remain committed through sprint, fire and weapon-switch requests.
Pack-a-Punch is a specific transfer exception; it preserves already-earned
events and does not interrupt the other gun's operation.

Normal play starts with M1911 7/56 and an empty second slot. The Box costs 950,
reveals after five seconds and excludes every already-owned family, including
reserved or upgraded guns. Pack-a-Punch costs 5,000 and processes for nine
seconds. Its clear front/side interaction area uses a configurable 200 cm
reach around the machine's front anchor. A ready gun returns automatically
to its original slot. **It is permanently lost without refund after its
15-second ready deadline.** Reachable collection wins exactly at the deadline;
later arrival loses it. Pause freezes these gameplay timers.

Each living victim damaged by a discharge earns 10 impact points, plus 100
for a body kill or 120 for a qualified head kill. A shell awards impact points
per victim, not per pellet. Head health damage uses 1.5×; head trauma alone
cannot execute an otherwise surviving target. After 15 damage-free seconds,
health recovers at 10% of current maximum per second. Red edges track missing health.

Natural drops use one 1% total roll per registered death and equal initial
type weights. Insta-Kill and Double Points last 30 seconds; recollection
refreshes their timers. Max Ammo fills available owned guns without changing
machine reservations. Uncollected drops last 30 seconds. These are provisional
gameplay values; full spread, penetration, scoring and lifecycle rules are in
[combat rules](Docs/CombatRules.md) and [survival tuning](Docs/Candidate06SurvivalRewards.md).

## Build and run

Install Git LFS, Unreal Engine 5.7.2, compatible Visual Studio C++ tools and a
Windows SDK. Use a writable checkout outside synchronized folders. To reproduce
the packaged gameplay source:

```powershell
git lfs install
git clone --branch codex/candidate06 https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git checkout --detach c2c422012f4f219fb5e63ac24294a6ba8428f999
git lfs pull
git lfs fsck
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate06
.\Scripts\Launch.ps1 -Candidate Candidate06
```

Output is `Packaged/Candidate06/Windows`. Extract the complete release folder
and run its root `ProjectONE.exe`, keeping runtime files, prerequisites and
notices together. Add `-Sandbox` to Launch for tools. Numeric validation uses
`Validate-Packaged.ps1`; the separate relocated-map fixture requires
`-Modes ONE06PortabilityCheck -Map /Game/ONE/Maps/Portability06`.

## Sources and review limits

[Provenance](Docs/Provenance.md), [pickup sources](ArtSource/Pickups/Candidate06/README.md),
[pickup audio](ArtSource/Audio/Candidate06/README.md), [existing motion](ArtSource/Characters/C05/README.md)
and [weapon sources](ArtSource/Weapons/Candidate04/README.md) identify editable
assets and importers. The three new pickup cues are designed synthetic sounds.
No Project Zero content, external asset pack, paid service or broad licensing
change is introduced.

Numerical checks, recordings, visual inspection, listening and native desktop
input are separate evidence. Their actual completed scope and limitations are
reported in the pass; none establishes user approval of game feel. The next
recommended milestone is the [infected visual/physical overhaul and recorded-audio replacement](Docs/Roadmap.md).
The six perk chambers follow later; neither milestone starts automatically.
