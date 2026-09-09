# Project ONE accepted development sequence

Candidate06 covers combat, survival, rewards and machine usability on the
existing foundation. It does not add archetypes, weapon tiers, a finished map,
a new production spawn system, the infected/audio overhaul, or perk chambers.

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

## Following milestone: infected redesign and recorded audio

- Replace the bald-mannequin look with dead, bloody, decayed humans: a convincing
  base archetype and compatible visual variants.
- Remove the permanently bent “T. rex” arms. Pursuit must flow into attacks
  without repeated full stops; build expandable attack selection with distinct motions.
- Use controlled physical response, crowd contact, stumbles/toppling and recovery
  instead of orbiting avoidance bubbles. Preserve navigation, limb-loss behavior
  and bounded physics.
- Use suitable recorded sources for gunfire, reload mechanisms, footsteps,
  clothing, casing contacts and performed creature voices. Electronic layers
  remain appropriate for deliberate upgraded effects, not every physical sound.
- Preserve editable references and provenance. Publish only sources whose terms
  permit the intended source distribution; do not buy content or silently add
  restricted packs, and do not call synthesis real recordings.

Candidate06's three pickup cues may use intentional stylized sound design. This
narrow permission does not authorize replacing all weapon/zombie audio now.

## Subsequent milestone: six perk chambers

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
require explicit tuning decisions; none is settled here. Do not automatically
begin these future milestones after completing Candidate06.
