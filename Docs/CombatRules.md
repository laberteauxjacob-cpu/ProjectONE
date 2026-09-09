# Candidate06 input and combat rules

This document describes the current source contract and editable defaults.
Builds, automation results, packaged checks, recordings, visual inspection and
audio review are separate evidence. Historical candidate reports continue to
describe their preserved builds.

## Cursor aim and height controls

LMB is the only firing button. The default aim plane is 125 cm above the
player's capsule floor. Holding RMB selects the 168 cm head plane; holding
Left Ctrl selects the 65 cm low plane. Left Ctrl takes priority when both
modifiers are held. Neither modifier fires by itself, and LMB remains usable
while either is held. The HUD identifies the selected height.

The ordinary cursor ray intersects that horizontal plane using the viewport
projection and the unmodified spring-arm camera. Enemy collision and world
hits do not choose the aim point. Moving an infected into the same scene can
change what a shot hits, but cannot select a different initial direction for
the same pose, cursor, camera, height and spread state. These planes follow
the player's floor elevation; they do not seek an enemy's head on different
terrain or track a moving anatomical target.

The character-centered cursor direction controls facing. Inside a 4 cm center
radius, the last valid direction is retained. Post-pose convergence uses the
evaluated muzzle and selected plane point, with 45-degree yaw and 35-degree
pitch bounds. A point behind the muzzle uses forward virtual convergence;
it cannot produce a backward shot. The gameplay aim projection excludes the
camera shake applied later for presentation.

Anatomical queries use the actual infected collision components. Current
head and torso capsule radii are 8.2 and 14 cm; lower/upper arm radii are 5.8/7
cm and lower/upper leg radii are 6.5/8 cm. A removed region cannot receive live
regional damage. These dimensions describe source geometry; its correspondence
to rendered bodies must be assessed in the separate standing-target checks.

## Spread and projectile travel

Every projectile receives one sampled direction from the weapon's private
random stream. Cosmetic randomness, enemy activity and power-up rolls do not
consume that stream. Even a zero-spread comparison consumes two samples.
Each shotgun pellet has its own sample; no pellet is redirected to an actor.

Spread combines base spread, movement contribution and per-instance firing
bloom, clamped at the effective maximum. Movement contribution scales with
horizontal speed relative to walk speed, capped at 1.5 times the listed
contribution. Bloom recovers after the last shot's recovery delay, including
while an owned weapon is holstered. Values below are degrees, with recovery
rate in degrees per gameplay second.

| Weapon | Base | Moving contribution | Growth per shot | Maximum | Recovery delay / rate |
|---|---:|---:|---:|---:|---:|
| M1911 | 0.25 | 0.25 | 0.035 | 0.65 | 0.15 s / 2 |
| M4A1 | 0.35 | 0.30 | 0.055 | 1.15 | 0.16 s / 2.5 |
| Remington 870 | 4 | 0.40 | 0.10 | 4.7 | 0.25 s / 3 |
| Last Word | 0.25 | 0.25 | 0.035 | 0.65 | 0.15 s / 2 |
| Overcurrent | 0.25 | 0.30 | 0.055 | 1.15 | 0.16 s / 2.5 |
| Gravebreaker | 4 | 0.40 | 0.10 | 4.7 | 0.25 s / 3 |

The shoulder-to-muzzle segment first checks physical world obstruction. It
does not damage infected on a separate direction. If clear, the sampled ray
extends backward along its own line to shoulder depth for close contact, then
continues forward. This gives each pellet one origin, immutable direction,
endpoint and range budget. There is no separate damaging prefix to duplicate
or terminate an otherwise penetrating contact.

Distance and falloff are measured from that original projectile origin,
including the collinear close-contact extension. They never restart at a
victim. Consecutive contacts are traced along the same ray; after an infected
contact, that whole actor is ignored for the remainder of that projectile.
Overlapping anatomical components therefore do not spend multiple body slots
or receive duplicate damage from one pellet. Different pellets can contribute
to different regions of the same victim, then resolve as one damage packet.

| Weapon | Damage per bullet/pellet | Pellets | Range | Falloff starts | Damage fraction at range | Total living bodies | Retained after each body | Extra-body range |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| M1911 | 28 | 1 | 2,400 cm | 1,000 cm | 55% | 2 | 60% | Full range |
| M4A1 | 32 | 1 | 2,800 cm | 1,400 cm | 65% | 3 | 65% | Full range |
| Remington 870 | 15 | 8 | 1,400 cm | 500 cm | 20% | 2 per pellet | 60% | 650 cm |
| Last Word | 56 | 1 | 2,400 cm | 1,000 cm | 55% | 3 | 65% | Full range |
| Overcurrent | 64 | 1 | 2,800 cm | 1,400 cm | 65% | 4 | 70% | Full range |
| Gravebreaker | 30 | 8 | 1,400 cm | 500 cm | 20% | 3 per pellet | 65% | 800 cm |

These are gameplay balance values, not claims about real ammunition. Falloff
is linear between its start and the range endpoint. Each prior living body
applies another retention factor. All six profiles require at least 2 remaining
damage for an accepted contact. A shotgun's shorter extra-body range applies
after its first living victim and still uses the original travel distance.
Actual damage, distance and cover can stop a ray before its body allowance is
used. Configuration is clamped to at most 5 living bodies, with at most 16
contact queries per projectile.

Ordinary solid world geometry stops the ray. Corpse impacts remain cosmetic,
do not consume the living-body allowance and cannot award points; traversal
still obeys the contact-query bound and remaining damage/range. Forward tracers
follow the actual ray endpoint. At most three shotgun tracers are drawn, and
contacts behind the muzzle do not create backward tracer lines. Shotgun crowd
damage comes only from the eight pellet paths and their penetration.

## Firing and reload commitment

An eligible pistol or shotgun tap gets exactly one post-pose dispatch,
including a tap released before that dispatch. A cooldown, pump, equip,
reload or handoff rejection never waits for a later eligible state. Dispatch
rechecks the same owned instance and current eligibility.

M4A1 holds establish a 0.100-second automatic interval, or 600 RPM.
Overcurrent uses 0.0869565 seconds, or 690 RPM. Frame quantization can vary
individual intervals. Only an established burst retains fractional timing
remainder; a hitch never creates several owed discharges in one frame.
Release ends the burst. Reload, equip, pause, input flush, handoff and death
disarm it, requiring release and a fresh eligible press.

Ordinary magazine reloads remain committed. Shift, fire, slot selection,
cycling, repeated R, ordinary acquisition and sandbox refill cannot cancel
them or bank a later switch. Movement and sprint remain available. Earned
magazine-out and magazine-in events occur once. Accepted Pack-a-Punch deposit
is the explicit transfer exception described below. Max Ammo has a separate
resupply transaction; neither makes an invalid fire press wait for completion.
Death and reset invalidate remaining callbacks.

An equipped empty weapon automatically reloads when reserve exists and its
pump obligation has completed. Holstered weapons do not reload themselves.
A deliberate press with both loaded and reserve ammunition empty produces
rate-limited dry feedback. Shotgun shells are earned only at their transfer
events. A press during shell loading with an earned shell and no pump obligation
closes the reload; another eligible press after the support hand returns is
required to shoot. The closing press is not a delayed shot. Required pumps
and spent-case obligations remain attached to their weapon instance.

## Damage, head kills and reactions

The baseline infected has 112 HP. Head health damage uses an editable 1.5
multiplier; arm health damage retains its 0.4 multiplier. A close base M1911
headshot therefore deals 42 HP and M4A1 deals 48 HP. Head trauma alone cannot
force an otherwise nonlethal hit to kill: head removal requires lethal head
health damage, an eligible head-dominant Insta-Kill, or an already dead target,
in addition to the trauma threshold. There is no first-hit survival shield.
Existing arm/leg loss rules and regional gore remain.

Headshot kill classification requires strictly more than half of the lethal
packet's effective health damage to come from the head. Exactly half does
not qualify. One incidental shotgun head pellet cannot turn a body-dominant
shell into a head kill. Earlier head damage does not qualify a later lethal
body discharge. Classification and cosmetic severing are distinct results.

Damage reports live hit, new kill, corpse hit or rejected. Only accepted live
hits and new kills refresh the hit marker, with a distinct kill shape. Blood
and regional wounds follow each actual victim transaction. Minor directional
reactions remain additive to pursuit and committed attacks; heavy reactions
use their threshold and cooldown. The base shotgun threshold is 70 raw packet
damage and Gravebreaker uses 60; pistol/rifle rows use 10,000. Death impulse
is clamped to 150–550, including explicit Insta-Kill, which never needs an
artificially huge damage packet.

Three attack families retain authored windup, bounded swept movement, one
contact and recovery. Heading commits at attack start. Walls, a missed arc,
missing required limbs and death prevent inappropriate contact. Strike damage
is 19, initial range is 88 cm and player damage protection is 0.55 seconds.
Player walk/run remain 225/370 cm/s; infected shamble/pursuit remain
100/195 cm/s.

## Player health and combat points

Player regeneration begins after 15 gameplay seconds without an accepted
health reduction and restores 10% of current maximum HP per second. At
100 maximum HP this is 10 HP/s; at 200 it is 20 HP/s. A frame crossing the
delay heals only for its eligible portion. Protected, zero, negative and
rejected damage do not restart the clock. New accepted damage interrupts
healing. Pause freezes recovery; death/reset clears its state and recovery
cannot revive. Infected do not receive player regeneration.

Persistent red screen edges follow current missing-health fraction, with a
separate brief accepted-hit pulse. They recede while health recovers and clear
at full health, death or a new run. The center remains available for aiming.

Each discharge awards 10 impact points per distinct living victim damaged.
One shotgun shell uses the same rule regardless of how many of its pellets
hit that victim. A new body kill adds 100; a qualified head kill adds 120
instead of 100. Thus a nonlethal bullet earns 10, a body kill 110, a head kill
130, and one bullet wounding a victim then body-killing another earns 120.
A shell damaging three living victims earns 30 impact points plus their
applicable kills.

GameMode owns run/discharge/victim award receipts and registered deaths.
Corpse, rejected, duplicate, closed and old-run events cannot award again.
Double Points is captured once at discharge start and multiplies both impact
and kill components, including all victims of that discharge. It does not
affect machine prices, developer grants or refunds. The small combat-gain HUD
batch reflects these combat awards separately from noncombat point changes.

## Mystery Box and Pack-a-Punch

The run starts with M1911 and at most two carried slots. Base and upgraded
variants are the same ownership family. Mystery Box purchases cost 950 and
keep the fresh F hold and approximately 5-second reveal. Selection happens
once among positive-weight families absent from all owned slots, including
holstered, machine-reserved and ready-to-return instances. Owned duplicates
are excluded from the pool rather than converted into ammunition.

An empty eligible pool disables purchase without a charge. A forced sandbox
reward that is owned or has zero weight also disables purchase with a reason.
If ownership changes while an already paid reward waits, the machine refunds
that receipt once and resets without rerolling. A valid ready reward still
uses a fresh nearby F hold, filling an empty slot or replacing the offered
available slot. A committed magazine reload must finish before Box collection.

Pack-a-Punch costs 5,000 and accepts a fresh F tap while the player is alive,
within its clear front/side interaction area and carrying an available base
weapon. The default area has a 200 cm radius around a machine-local front
anchor and excludes the far rear; it follows machine rotation and placement.
An empty magazine, reload, shell loading or pending pump does not disqualify
deposit. One existing reservation, a handoff, insufficient points or an already
upgraded weapon does. One upgrade tier remains the limit.

Accepted deposit settles only already-earned operation events, snapshots and
reserves the exact instance/slot, charges once and begins the 9-second process
immediately. It does not wait for the old reload or transfer animation. Any
other available weapon remains usable after the brief handoff. Depositing the
only available weapon can leave the player unarmed; the prompt states this.

When ready, the upgrade returns automatically to its reserved slot whenever
its living owner is in the valid interaction area. No F press or intake-center
alignment is required. Return does not interrupt another weapon's reload or
held fire. Presentation/equipping follows the returned slot's availability.
The machine holds a completed upgrade for 15 gameplay seconds, showing a
countdown and a warning during the final 5 seconds. Reachable collection wins
an observation exactly at the deadline; an observation after the deadline
cannot rescue it.

If the deadline expires, that reserved instance is permanently lost, its slot
becomes empty and its family can roll again. There is no gameplay-loss refund.
Completed delivery and expiry close payment receipts permanently. Genuine
technical failure uses the separate same-run rollback/refund path, preserving
the original instance and already-earned operation state; stale callbacks
cannot duplicate a delivery, refund or weapon.

## Dropped power-ups and lifecycle

Each eligible registered infected death makes one 1% total drop roll. A success
selects one of Insta-Kill, Double Points or Max Ammo using equal default weights.
This is not three independent 1% rolls. The drop stream is separate from weapon
spread. Forced sandbox drops have separate counters and consume no random draw.

| Pickup | Effect |
|---|---|
| Insta-Kill | A valid living-victim contact becomes explicitly lethal for 30 gameplay seconds; range, cover, pellet paths and damage acceptance still apply. |
| Double Points | Both combat impact and kill awards are doubled for 30 gameplay seconds. |
| Max Ammo | Available carried weapons receive their effective loaded capacity and reserve limit immediately. |

Same-type timed recollection refreshes to 30 seconds without stacking; the two
timed effects coexist. Max Ammo does not create ownership or alter an instance
inside a machine. It reconciles earned ammunition events, disarms held input,
continues a magazine's closing phase or closes shell loading, and preserves
actual pump/spent-case obligations without duplicate ejections or transfers.

An uncollected pickup lasts 30 gameplay seconds. Placement searches a bounded
set of nearby ground positions, requires a complete navigable path from the
player and rejects occupied space. Collection requires the current living
player, a real capsule overlap within the default 80 cm sphere and clear world
reach. Standing in the valid overlap collects automatically, without F.
World expiry wins its exact collection deadline.

The default cap is 8 pickup actors, including the 1.05-second collected-sound
tail; the editable cap is bounded to 1–32. A successful probability roll that
cannot spawn records cap, ground or spawn failure separately from chance misses.
Timed effects and world lifetimes pause with gameplay. Death, restart and
teardown invalidate run ownership, clear modifiers and remove pickups/cues.
Maps need navigable ground; no arena coordinate or level-name fallback creates
a drop. Normal rounds use authored `ONE_Spawn` target points and wait with a
clear diagnostic if none exist.

## Presentation, UI and verification

Weapon shake is a bounded camera translation applied after logical aiming.
Base impulses are 0.55 for M1911, 0.32 for M4A1 and 1.15 for Remington 870;
upgrades multiply these by 1.15. Actual multi-kills and accepted live shotgun hits can add
a small accent. The total amplitude is capped at 1.8 and decays exponentially.
It does not alter shot direction, collision or cursor intent. The H tray cycles
100%, 50% and Off through `one.CameraShake.Strength`; death clears amplitude.

H opens the help/sandbox tray and F1 toggles encounter mode. UI actions require
press and release on the same enabled button in the same context, consumed
before gameplay firing. Pause, death, reset and UI transitions flush held input.
The pointer replaces the crosshair while a menu or tray is active. The tray
includes three explicit forced-pickup buttons without new global hotkeys.

Weapon, infected and ambience gains remain editable through
`one.Audio.Weapons`, `one.Audio.Zombies` and `one.Audio.Ambience`. Held upgrade
effects retain one mesh and one bounded nonshadow light; base, holstered,
unowned and handoff-suppressed states disable them. Pickup emblems, timed
warnings and collection cues present the actor's authoritative state.

Automation and runtime fixtures exercise different parts of this contract.
Pure ballistic/receipt tests do not establish actual cursor collision, machine
reach, packaged behavior or visual/audio quality. The Candidate06 pass record
must identify the actual executed checks and their source revision separately.
