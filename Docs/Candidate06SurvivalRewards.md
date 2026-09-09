# Candidate06 survival and rewards tuning

Implementation defaults; runtime and visual verification are reported separately
in the Candidate06 pass record.

| Rule | Initial value |
|---|---|
| Accepted player-damage recovery delay | 15 gameplay seconds |
| Recovery rate | 10% of current maximum health per second |
| Living-victim impact | 10 points per distinct victim per discharge |
| Additional body kill | 100 points |
| Additional qualified head kill | 120 points instead of 100 |
| Double Points | Both combat components multiplied by 2 once |
| Natural drop chance | One 1% total roll per registered death |
| Conditional type weights | Insta-Kill / Double Points / Max Ammo = 1/1/1 |
| Timed effect duration | 30 gameplay seconds; same-type recollection refreshes |
| Uncollected world lifetime | 30 gameplay seconds |
| World actor cap | 8, including the brief collected-audio tail; editable 1–32 |
| Collection sphere | 80 cm radius, requiring real player-capsule overlap and clear world reach |
| Combat HUD gain batch | 0.18 seconds |

Regeneration is explicitly enabled by the player; the shared health component
defaults to no recovery. Only a damage call that reduces health restarts its
clock. A frame that crosses the 15-second boundary heals only for the portion
after that boundary. Maximum-health changes use the current maximum without
starting a parallel health system. Death, reset and component teardown discard
old recovery state. Pause contributes no gameplay elapsed time.

One shotgun shell awards 10 impact points per distinct living victim, regardless
of pellet or overlapping anatomical-region count. Damage remains independent
of that scoring policy. One nonlethal bullet awards 10; a lethal body hit awards
110; a qualified lethal head hit awards 130. A penetrating bullet that wounds
one and body-kills another awards 120. A shell damaging three victims awards 30
plus applicable kills. Earlier head damage does not itself qualify a later
body-dominant lethal event; qualification comes from the actual lethal damage
transaction.

GameMode issues one run/discharge modifier snapshot and receives each final
victim outcome. Live awards require a registered living victim; new kills require
the same active discharge's authoritative registered-death receipt. Corpse,
rejected, repeated, closed and old-run events cannot award. Receipt memory is
bounded and eviction does not restore eligibility. The synchronous integration
closes every discharge after its victim loop. Developer grants, refunds and
machine prices never enter this multiplier path. Completed/expired machine
receipts become terminal and cannot later be refunded.

At zero starting points, nine 110-point body kills reach 990 for a 950-point Box
roll; seven 130-point head kills reach 910 and eight reach 1040. Nonlethal impacts,
penetration and shell crowds can reach the price sooner. A 5,000-point upgrade
alone requires 46 body kills (5060) or 39 head kills (5070) at those simplified
single-impact totals. These examples exclude other impacts and Double Points;
prices remain 950/5,000 and the actual route depends on purchases and combat.

Drops use their own random stream. A successful chance roll selects one weighted
type; it does not run three 1% rolls. A passed roll that cannot create an actor
records cap, unreachable-ground or spawn failure separately from chance misses.
Forced sandbox attempts have separate counters and do not consume random draws.
Default weights and chance are editable on the GameMode power-up component.

Placement searches 13 bounded nearby offsets, requires navigable ground connected
to the player, rejects occupied volume and checks clear vertical access. No
arena coordinates, level names or corpse actor names are used. Maps need usable
navigation; an unreachable or navigation-free drop is explicitly suppressed.
Collection validates the current run and living player, actual capsule overlap,
and a clear segment through world geometry. Expiry wins at the exact deadline.
Timed effects freeze while paused and clear on death/reset. Max Ammo delegates
to the existing weapon component's explicit replenishment transaction; it never
edits machine rollback snapshots or creates ownership.

The three power-up motifs, restrained final warning and collection sounds belong
to the visual component; collision, lifetime and effect ownership remain in the
pickup actor and GameMode component. The active actor cap includes collected
actors until their short cue tail is destroyed, keeping simultaneous voices
bounded during repeated forced tests.
