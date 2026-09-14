# Project ONE accepted development sequence

Candidate07 is the current technically verified Windows prerelease, with three
integrated infected appearances, an infected-specific motion bank, living
contact/fall/recovery and recorded physical audio. Native desktop capture was
blocked; appearance, feel and perceptual audio approval remain provisional.
[CurrentState](CurrentState.md) and the [Candidate07 pass](Passes/Candidate07.md)
separate completed checks from ordinary encounters, performance, native play,
packaged testing and publication gates. No new gameplay archetypes, weapon
tiers, finished map, production spawn system, perk chambers or menu overhaul
are part of this pass.

## Reuse and future map entry

Keep machines, pickups and editable power-up definitions, player health/modifiers,
scoring events, weapon profiles and encounter integration reusable. New behavior
must not depend on arena coordinates, actor names, bench locations or copied
level scripts. Query owned slots; retain two runtime slots while allowing future
Mule Kick cleanly. Candidate06's tiny relocated-system test level is a dependency
check, not the beginning of a finished map or a clone of the arena.

Production maps must spawn infected outside the player's view or behind real
occlusion, then route them through authored entrances:
**hidden spawn → inaccessible/occluded approach → visible entrance → pursuit**.
Windows, wall breaches and crawls beneath objects are future entrance examples,
not assets for this pass. Visible developer spawning is a disclosed sandbox
testing exception, not the production solution.

## Candidate07: implemented, verification in progress

- Maintenance, Laboratory and FacilityStaff have distinct modeled clothing,
  faces and hair on the retained infected rig/cut contract. They share gameplay
  statistics; these are visual variants, not new enemy classes.
- Fourteen infected-only clips support pursuit, turns, three attack families,
  reactions and separate prone/supine recovery. Attack selection considers
  available limbs and approach conditions; physical responses preserve health,
  registered living identity and current-pose dismemberment.
- The audio bank contains 134 processed cues and 76 retained recorded source
  files. Physical events use recordings with documented firearm/Foley
  analogues; deliberate upgraded energy overlays remain synthesized. Source
  licensing, reproducible processing, runtime dispatch and listening judgment
  are separate requirements.
- The implementation is undergoing actual encounter, performance and release
  verification. Bounded source/engine still reviews are not continuous playback,
  audio audition, native free-play or user approval.

Editable sources and import order are linked from the
[art-source index](../ArtSource/README.md). Candidate06 pickup cues and machine
reward cues retain their deliberate earlier sound design. Existing source
notices remain attached; no broad project license or additional content
purchase is implied.

## Proposed next correction: floor-level mouse targeting

The actual motion check reproduced ordinary Left Ctrl + LMB shots passing
above a corpse below the fixed 65 cm low-aim plane. Evaluated regional queries
can reach that anatomy, but the current mouse-height controls do not follow it.
After Candidate07, propose a focused targeting/usability pass that preserves
target-independent aiming and makes intended floor-level shots reviewable.
No snapping, larger hit regions or control redesign has been added to conceal
the limitation. This is a recommendation for a separately authorized task;
[the runtime note](Candidate07Infected.md#queries-and-controls) records the
measured limitation.

## Future milestone: six perk chambers

| Perk | Accepted role |
| --- | --- |
| Juggernog | Extra maximum health |
| Speed Cola | Faster coherent reloads |
| Double Tap | Increased firing rate with synchronized mechanisms |
| Quick Revive | Solo self-revive |
| Stamin-Up | Faster sprint |
| Mule Kick | Third carried slot |

The player physically enters a chamber and is protected during a legitimate
upgrade cycle. On exit, bounded blast damage and knockback create escape room.
Invalid or repeated purchases must not grant free invulnerability. Integrate
with shared health, score, inventory, modifier and machine ownership interfaces.

Prices, exact strengths, durations, revive consumption and perk-loss rules still
require explicit tuning decisions; none is settled here. Perk chambers and
menu work remain future tasks outside Candidate07. Stop after the current
assignment; do not automatically begin either or the proposed targeting pass.
