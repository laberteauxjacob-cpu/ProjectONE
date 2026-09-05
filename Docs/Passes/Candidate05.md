# Candidate05 — responsive combat and presentation

This checkpoint contains the integrated implementation, functional verification
and initial rendered review. Publication and final acceptance evidence remain pending.

## Changes driven by playtest

| Reported issue | Candidate05 behavior |
| --- | --- |
| Near cursor reverses shots/traces | Shared character-centered intent, four-centimeter fallback, exact valid forward convergence and bounded contact traces; real cover remains authoritative |
| Clicks execute after the gun becomes available | Only an eligible press gets one post-pose dispatch; rejected input is discarded |
| Rifle feels slow and separated | M4A1 0.100-second interval and matching fire operation; Overcurrent retains its 15% rate advantage |
| Reload interruptions leave confusing states | Magazine reloads commit through sprint/fire/switch requests; inventory and machine backdoors reject until a fresh valid request |
| Shotgun loading/input ambiguity | Earned shells remain; closing the load requires safe hand return and a fresh eligible shot, with no hidden queue |
| Weak first-hit response | Animated live-hit marker, distinct kill shape, directional blood/audio and additive localized reactions; corpse hits cannot award another kill |
| Deep crouching and mechanical strides | Eighteen revised player walk/run/turn clips, less knee compression and restrained torso weight shift on the existing rig |
| Zombie stops and waves an arm | Stepping swipe, cross-body rake and two-hand families with anticipation, fixed heading, bounded step, contact and recovery |
| Small glyphs and permanent text panels | Original rounded artwork, prominent ammo/yellow points, health bar, actual weapon images, compact slots and contextual labels |
| Permanent sandbox paragraph | Compact badge and grouped H tray; existing F1 meaning remains |
| Box reward unclear from camera angle | Reel uses actual assembled weapon images and the existing paid preview/result; ready state shrinks and directs return to the box |
| Plain pause/death rectangles | Shared style, actual scene dimming, large results and mouse actions; death beat is brief and immediately skippable |
| Silent environment/enemies | Original spatial mechanical room beds, sparse cues and varied breathing/pursuit/attack/hit/death sounds with bounded groups |
| Upgrades lose identity while carried | Close violet/cyan/ember rails and light follow the held gun; stronger flash/tracer colors and layered energy shot variations |

Implementation alone is not visual or auditory acceptance.

## Retained balance and tuning

| Weapon | Loaded / initial reserve | Damage before regional modifiers | Configured interval | Nominal burst DPS before falloff/reload |
| --- | ---: | ---: | ---: | ---: |
| M1911 | 7 / 56 | 28 | 0.240 s | 116.67 |
| Last Word | 14 / 112 | 56 | 0.208696 s | 268.33 |
| M4A1 | 24 / 192 | 32 | 0.100 s | 320 |
| Overcurrent | 36 / 384 | 64 | 0.086957 s | 736 |
| Remington 870 | 6 / 36 | 8 × 15 pellets | 0.780 s including fire/pump | 153.85 if all pellets hit |
| Gravebreaker | 8 / 72 | 8 × 30 pellets | 0.678261 s including fire/pump | 353.85 if all pellets hit |

M4A1/Overcurrent previously used 0.160/0.139130 seconds: nominal burst DPS
200/460. Both rise by 60% from the explicit cadence change. Damage, capacities,
reserve limits, spread/recoil response, upgrade multipliers and special behavior
otherwise retain Candidate04 tuning. These are ideal burst calculations, not
measured practical output. Last Word retains one additional victim at 60% damage;
shotgun regional pellets aggregate once per victim/discharge.

Player walk/sprint remain 225/370 cm/s, infected shamble/pursuit 100/195 cm/s.
Attack damage remains 19, initial range 88 cm and player protection 0.55 seconds.
Box/PAP prices remain 950/5000; cycles remain five/nine seconds. Exactly two
owned slots and one upgrade tier remain. Ready rewards wait indefinitely.

Swipe contact is at 0.45 seconds in a 0.96-second clip with an 18 cm maximum
step; rake at 0.48/1.08 seconds with 12 cm; two-hand at 0.54/1.12 seconds with
14 cm. Steps end before contact, sweep for blocking geometry and do not track
through the player during commitment. Required limbs, wall/arc checks, death
and one-delivery state still control actual damage.

## Verification status

Local Editor and Win64 builds succeed. Seventeen native tests pass, including
eight warning-free Candidate05 cases. Three historical fixtures retain five
teardown warnings. An initial attack fixture lacked its engine world context;
that fixture was corrected and rerun without removing assertions or suppressing
errors.

Weapon runtime checks pass 191 assertions at each requested 30/60/120 FPS.
[Timing details](../../Evidence/Candidate05/WeaponTiming.md) retain actual cadence,
frame quantization and latency. The probes exercise all six variants, eligible
short taps, rejected clicks, automatic release/interruption, committed reloads
and inventory guards, earned shells, auto-reload, dry fire and a deliberate hitch.
Loadout setup, fixture health restoration and headless operation are disclosed.
Native desktop input, imagery and performance remain separate gates.

The first arena aim runs passed 812 and 1,988 checks, but their elevated target
fixture concealed the standing torso's real height. Actual rendered pursuit
exposed repeated misses. Those results are preserved as superseded fixture
coverage. Corrected standing-height and projected-cursor probes pass 2,612 and
2,564 checks respectively, each with 480 actual discharges. The first uses
NullRHI; the second uses an actual 1600x900 offscreen viewport and production
mouse picking. Both run fixed 1/60-second simulation steps with uncapped
execution. These are collision checks, not real-time cadence or native input.
The changes preserve exact forward convergence, interior region selection and
a per-pellet collision prefix bounded by the actual barrel plane. Failed test
iterations also exposed mismatched expected cursor direction and a low-cover
fixture whose close actor won the physical obstruction trace first; those
assumptions were corrected without changing the collision priority.
Two native aim tests also cover 1,008 mathematical variant/heading/radius cases;
those calculations are not rendered trajectory evidence.

All 82 new assets import and their engine references verify. The first verifier
mistook two agreeing serialized import-metadata copies for one malformed record;
the reader now validates each encoded FString and requires agreement. No asset
rewrite was needed to make that check pass.

Rendered UI checks pass 49 assertions at each of four actual sizes: 1280x720,
1600x900, 1920x1080 and 2560x1080. Six updated 1600x900 states have been visually
reviewed. The latest production-input motion recording passes 80 checks with
1,208 frames and 51.37 seconds of real engine audio. Rapid pivot foot clearance
is 5.898/7.320 cm; this measures lift, not a claim of natural motion by itself.
The approaching target produces live hits, one kill and a later cosmetic corpse
hit. Earlier failed motion attempts remain preserved, including low clearance
and the standing-target aiming defect. [Visual review](../../Evidence/Candidate05/VisualReview.md)
covers 496 consecutive frames across all 16 strides, turns, attacks and hit
outcomes, with mechanical joints and similar swipe/rake silhouettes retained as
limitations. The combined six-weapon/machine/menu recording passes 93 checks,
with 4,635 frames and 197.29 seconds of actual engine audio. Its visual review,
auditory review, recording-free rifle/crowd profiles and fresh public package
verification remain separate gates.

## Source and evidence

[Combat rules](../CombatRules.md), [character pipeline](../CharacterPipeline.md),
[provenance](../Provenance.md), [motion](../../ArtSource/Characters/C05/README.md),
[HUD](../../ArtSource/UI/Candidate05/README.md) and
[audio](../../ArtSource/Audio/Candidate05/README.md) retain original editable
sources and import steps. Records are indexed in
[Evidence/Candidate05](../../Evidence/Candidate05/README.md).

Candidate04 and earlier tags, releases and packages remain preserved. The exact
clean-checkout packaged source, later evidence revision, checksums and public
download verification will be recorded after those actions complete.
