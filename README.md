# Project ONE — Candidate03

An original solo, top-down facility survival prototype in Unreal Engine 5.7.2.
Candidate03 improves directional movement, reload intent, weapon presentation,
regional damage, physical remains and blood within the existing two-weapon arena.

The verified gameplay/package source is
[`e81e1137ed891570c73c8c278fc2c8cc2250bd04`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/e81e1137ed891570c73c8c278fc2c8cc2250bd04).
Its clean public checkout built the editor, Win64 game and complete Development
package. All five native tests and all 13 packaged modes passed; the packaged
modes logged 714 PASS assertions and zero failures. A separate rendered
physicality capture passed 249 assertions and supplied 3,069 genuine frames.
Four separately measured performance scenarios are complete. These results
are distinct from visual-direction approval and perceptual audio review.

Play the [Candidate03 prerelease](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
or inspect the [review branch](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/codex/candidate03)
and [Candidate02 comparison](https://github.com/laberteauxjacob-cpu/ProjectONE/compare/candidate02...candidate03).
The immutable `candidate03` reference includes later documentation/evidence;
the gameplay and package source remains the exact commit above.
[Native input review](Evidence/Candidate03/StageE/native_input_review.md)
covers both weapons, reloads, selection, lighting, pause, death and a separate
normal launch with sandbox/reset/restart controls. Candidate02 remains available
as a [preserved rollback](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate02).

See [CurrentState](Docs/CurrentState.md), the
[Candidate03 pass report](Docs/Passes/Candidate03.md) and
[evidence index](Evidence/Candidate03/README.md) for scope and limitations.

## Controls and current play

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

Player walk/sprint speeds are 225/370 cm/s; infected shamble/pursuit speeds are
100/195 cm/s. Eight authored directions and stepping turns retain independent
aim. An empty equipped weapon automatically reloads when ready and reserve is
available. Held Shift defers manual and automatic reload, even while stationary.
Interrupted reloads retain earned ammunition; switching retains both slots'
ammunition and the shotgun's pending pump/ejection obligation.

The carbine starts with 24 loaded rounds and 192 reserve; the shotgun has six
shells and 36 reserve. Normal rounds begin after five seconds. Later rounds add
48 reserve carbine rounds and eight shotgun shells, within their caps. There is
one infected archetype, 100 points per registered kill and at most 18 active
enemies. Sandbox disables automatic waves.

Pellets retain body/head/left-arm/right-arm/left-leg/right-leg damage and spatial
data before one transaction per victim. Head, either arm and the left leg can
detach; the right leg takes damage and bleeds but is not severable. Head loss,
left-leg loss or losing both arms is fatal. Death and detached parts begin from
the evaluated pose with skeletal physics. Finite wounds produce projected
trails and growing pools. Supported, quiet remains can hold their measured pose
as collidable kinematic bodies; fresh corpse damage/severing resumes physics,
while ordinary contact alone leaves those held remains fixed.

## Verified results and limits

- [Fresh build](Evidence/Candidate03/StageE/fresh_build.json): 569 tracked files
  and 291 hydrated LFS payloads verified at the built source. Native tests:
  two successes without warnings, three with five isolated test-world teardown
  warnings, zero failures.
- [Packaged checks](Evidence/Candidate03/StageE/packaged_checks.json): all 13
  modes exited successfully with their own zero-failure completion markers.
  The 714 logged PASS assertions retain 208 categorized initialization warnings;
  no Error, Fatal or ensure was recorded. Assertion totals are not counts of
  distinct test cases.
- [Motion](Evidence/Candidate03/StageE/physicality_motion_review.md),
  [blood/crowd](Evidence/Candidate03/StageE/physicality_blood_crowd_review.md) and
  [rest](Evidence/Candidate03/StageE/physicality_rest_review.md) reviews use
  explicitly listed intervals from the final 249-check capture. The earlier
  244-check editor capture remains historical evidence.
- [Performance](Evidence/Candidate03/Performance/README.md): three 3,000-frame
  living-crowd runs at 6/12/18 enemies and one 14,038-frame physicality run.
  At 1600×900 with a 120 FPS cap on Ryzen 9 5950X / RTX 3090, full living-run
  means were 8.3754/8.3818/8.3861 ms. In the primary 10–20-second comparison,
  18-enemy p99 increased from 8.7933 to 9.0602 ms. Screenshot-associated spikes
  above 100 ms remain included. The separate 257-assertion, recording-free
  physicality profile averaged 8.3388 ms over 117.0604 seconds, with five frames
  above 16.7 ms and a 31.4258 ms maximum. These are single-run capped
  measurements, with documented ambient-application limits, not uncapped capacity.
- [Engine audio measurements](Evidence/Candidate03/StageE/packaged_comparison_audio.json):
  the 20.992-second recording covers both weapon phases, with zero full-scale
  samples. Perceptual listening and mix approval remain unavailable.

Characters still have mechanical low strides, compressed knees, crossing limbs
and awkward propped corpse poses. Crowds and racks obscure contacts; corner
cameras expose black off-map space. Small props/cases remain subtle. The
available native automation did not deliver sustained W/Shift input; held-input
behavior is covered by production input-dispatch tests, without a claim of
sustained native keyboard playtesting. Numerical checks, saved-frame inspection
and direct input are separate evidence.

## Play or build

The [playable archive](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate03/ProjectONE-Candidate03-Windows.zip)
is `ProjectONE-Candidate03-Windows.zip`:
**398,043,452 bytes**, SHA-256
`ada689b2314d3047d581d0523a2cbcd91f57661d06554d6288a6b4fae79600d2`.
The [archive audit](Evidence/Candidate03/StageE/archive_audit.json) verifies its
1,217 entries and binding to the [audited runtime](Evidence/Candidate03/StageE/release_audit.json).
Extract a complete Windows archive and launch its root `ProjectONE.exe`; keep
all extracted files together. Windows prerequisites are included. Players do not
need Unreal Editor or Blender, and ordinary play needs no validation flags.

Git LFS is required for Unreal Content, editable Blender sources and original
audio. Use a writable directory outside synchronized folders and explicitly
select the verified source before building:

```powershell
git lfs install
git clone https://github.com/laberteauxjacob-cpu/ProjectONE.git
Set-Location ProjectONE
git checkout --detach e81e1137ed891570c73c8c278fc2c8cc2250bd04
git lfs pull
git lfs fsck
```

Install Unreal Engine 5.7.2, Visual Studio C++ build tools and a compatible
Windows SDK. The verified toolchain used Visual Studio 2026 / MSVC 14.50.35725
and Windows SDK 10.0.26100.0; Unreal warns that this compiler is newer than its
preferred 14.44 release. Engine/compiler installation is separate from the repo.

```powershell
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Scripts\Build.ps1
.\Scripts\Build.ps1 -Game
.\Scripts\Package.ps1 -Candidate Candidate03
.\Scripts\Validate-Packaged.ps1 -Candidate Candidate03
.\Scripts\Launch.ps1 -Candidate Candidate03
```

`Build.ps1` defaults to the editor target. Build/package scripts accept
`-EngineRoot` or `UE_ROOT`, with the standard Epic Games UE_5.7 installation as
fallback. Packaging builds/cooks to `Packaged/Candidate03/Windows`; it does not
replace Candidate02. Launch and validation default to Candidate03.
`Launch.ps1 -Sandbox` opens the sandbox; `Launch.ps1 -Candidate Candidate02`
opens a retained local Candidate02 package if present. Dense captures and CSV
profiles are separate from the default 13-mode validation suite.

## Editable sources and preserved candidates

- `Source/ProjectONE`: gameplay, weapon definitions, native animation and checks.
- `Content/ONE`: imported runtime assets and the Containment map.
- [Character sources](ArtSource/Characters/Candidate03/README.md): directional
  locomotion, turns and modular infected on the accepted rig.
- [Weapon presentation inventory](ArtSource/Weapons/Candidate03/presentation_inventory.json)
  and [Candidate02 weapon sources](ArtSource/Weapons/Candidate02/README.md):
  original weapons, reload actions, brass and muzzle presentation.
- [Audio sources](ArtSource/Audio/Candidate03/README.md): six original firing
  profiles per weapon without immediate repeats, plus separate operation events.
- `Scripts`: generation, import, sanitation, auditing, validation and packaging.
  Unreal audio import commandlets require `-AllowCommandletAudio`.

Blender 5.1.2 is needed only for art authoring/regeneration. The
[provenance](Docs/Provenance.md), [character pipeline](Docs/CharacterPipeline.md)
and [environment pipeline](Docs/EnvironmentPipeline.md) document the originals.
No external sound pack or Project Zero content was used. No broad code/art license
has been selected, and no paid storage settings were enabled. Required source
assets use LFS; normal upload/fetch verification is separate from unavailable
account-specific quota/budget information.

Candidate02 source `aa4d55d04bf375bbef41362af77eec10d9ea224f` and packaged gameplay
revision `0637288d32b6fbebc67ef93d4f03e439ff38bb67` remain preserved with their
[pass report](Docs/Passes/Candidate02.md) and [evidence](Evidence/Candidate02).
Candidate01 and private local rollback history remain separate. The
[first Candidate03 source failure](Evidence/Candidate03/StageE/first_source_validation.md)
is retained as a failed historical suite; the later S2 pass does not relabel it.
