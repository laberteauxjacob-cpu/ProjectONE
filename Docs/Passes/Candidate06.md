# Candidate06 — combat, survival, rewards and machine usability

Status: implementation and development validation in progress. Candidate05
remains the latest verified public playable release. This page will identify
the final source, clean build, packaged tests and publication once completed.

## Scope

Continue the accepted Project ONE foundation. Preserve all previous candidates.
Change target-dependent aiming, spread/penetration, headshot damage and score,
player recovery, Box duplicate policy, Pack-a-Punch usability/deadline, three
pickups, health feedback and restrained camera shake. Do not implement the
following infected/audio or perk-chamber milestones; see [Roadmap](../Roadmap.md).

Left mouse remains the only fire button. Hold right mouse for head height or
Left Ctrl for low height; without either, aim at torso height. Low takes priority
when both are held. The wheel still cycles weapons. Pack-a-Punch uses a tap of F
to deposit and automatically returns a ready upgrade within reach. A ready gun
left uncollected for 15 seconds is permanently lost without a refund.

## Development checks so far

- Combined Editor build passes after correcting integration errors. This is a
  local development build, not a public package or fresh-clone result.
- The original three PCM pickup cues pass source checks and import successfully.
  The pickup material graph is created and checked. Auditory approval remains
  unverified.
- All 28 engine automation tests pass, with warnings in three historical
  fixtures. Updated fixtures preserve head-trauma and transfer boundaries.
- A separate small Portability06 map has 24 authored actors, relocated machines
  and built navigation. Its development runtime check passes 34 assertions.
  The final development survival/rewards recording passes 100 assertions,
  including a protected second attack and an actual pistol trace after collecting
  Insta-Kill and Double Points: one kill and exactly 220 points. It finalizes
  1,871 frames and 67.200 seconds of engine audio.
- A recorded six-weapon combat run passes 1,728 assertions, including actual
  standing head/leg contacts, modifier-only input, trace awards, cover,
  penetration and unchanged logical aim during measured render shake.
  Its 1,905 viewport frames and 70.421-second engine WAV finalize successfully;
  assembler input validation passes. This is development footage, not the final
  source-bound package recording or an audio audition.
- Grouping passes 920 assertions across all 18 distance/occupancy pairs,
  108 discharges and 234 actual projectile paths. Matched groups overlap with
  and without an infected; three-distance wall spans and spread recovery are
  recorded. The corrected setup uses the ordinary initial reserve amount.
- The expanded six-weapon combat check passes 2,109 assertions. It adds actual
  lethal head shots against declared wounded targets and three normal 112 HP
  crowd targets. In this fixture, the 870 kills the first and leaves the second
  at 40 HP; Gravebreaker kills two and leaves the third at 46.09 HP. These are
  actual seeded outcomes, not guaranteed results for every arrangement.
- The machine progression check passes 95 assertions. A 120-shot projected
  close-range/cover run across six variants passes 692 assertions; full-circle
  packaged projection remains pending.
- A recording-free development shotgun/six-enemy profile completes 38 checks
  and writes 3,684 engine CSV frames. Final packaged profiles remain pending.

Runtime fixtures, real-time recordings, visual inspection, audio listening,
recording-free performance and native input are separate forms of evidence.
Development fixture outputs and raw logs stay under ignored Saved. Required
final evidence will report actual scope and remaining limitations.
