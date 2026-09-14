# Candidate07 infected contract

This is the current implementation record for the in-progress Candidate07
milestone. Maintenance, Laboratory and Facility Staff are integrated in the
Editor game. Final combined review and packaged/public verification remain
pending. See [checkpoint status](Passes/Candidate07.md) for actual results.

## Character and rig

The appearance data asset supplies five meshes, five physics references,
fourteen infected-only animation sequences and a voice-variation salt. It has
no health, speed, damage, reward or weapon settings. Ordinary spawning selects
from `AONEZombie::GetProductionVariantPaths`; an explicit sandbox appearance
override uses the same navigation, collision and registration path.

The existing infected skeleton retains 21 Blender bones plus the imported
armature node. Imported anatomical left uses `_r` suffixes. The player retains
its own mesh, bind, animation graph and weapon-grip assets.

| Anatomical piece | Attachment / physics root | Cut contract |
| --- | --- | --- |
| Head | `head` | Paired surface at component Z 157.8 cm |
| Left arm | `upperarm_r` | Paired upper-arm plane; elbow/hand remain on detached piece |
| Right arm | `upperarm_l` | Mirrored paired upper-arm plane |
| Supported left leg | `thigh_r` | Paired plane near component Z 79.04 cm; existing stump path |
| Core | Shared complete pose skeleton | Matching surviving rims; no geometry bridge into removed pieces |

The exact source planes, weights, mesh identities, seam checks and head-fit
measurements live under `ArtSource/Characters/Candidate07`. Appearance changes
must repeat them. Hair and garment geometry must belong to the correct piece.
The head query is fitted to skin geometry, not to decorative hair volume.

## Movement, attacks and contact

Player speeds remain 225/370 cm/s, infected speeds 100/195 cm/s, infected health
112, strike damage 19 and player protection interval 0.55 seconds. Presentation
and attack selection use separate random streams from spread and pickups.

The infected graph explicitly samples idle, walk, run, two turns, five sided
attack performances, heavy hit, stumble and two recoveries. It advances gait
from actual speed and authored stride, and evaluates grounded foot contacts.
Explicit phase crossing delivers motion/audio events; imported animation
notifies are not assumed to fire from sequence evaluators.

| Family | Performances | Duration | Contact | Maximum committed step |
| --- | --- | --- | --- | --- |
| Forward swipe | Left / right | 0.96 s | 0.45 s | 18 cm |
| Cross-body rake | Left / right | 1.08 s | 0.48 s | 12 cm |
| Short two-handed strike | Both hands | 1.12 s | 0.54 s | 14 cm |

Selection considers available limbs, bearing, distance, entry speed and the
previous performance. Entry travel carries bounded approach momentum. Heading
stays committed through contact/follow-through; late recovery can blend into
pursuit. Each attack delivers at most one health contact, checks solid cover,
and cancels invalid contact after a real interrupt, missing limb or death.

The capsule transports the living character's kinematic pelvis and legs.
Physical animation allows the upper body to yield to contact. These skeletal
bodies do not supply a second weapon-damage path. Motor strength, impulses and
contact cooldowns are bounded. Meaningful measured closing speed/pressure can
produce a stumble or a full living fall; idle timer randomness cannot.

At most four living actors use full-body fall simulation simultaneously. A
fall disables that actor's transport capsule, retains health and registration,
and cancels its pending attack. Settled bodies attempt a local floor/clearance
solution. Blocked attempts are spaced out rather than moved to a remote spot.
Support must block the transport capsule. Pickup collection triggers cannot
act as floors or get-up obstacles; clearance still includes solid blockers,
pawn capsules and other infected's actual-pose anatomical queries.
Recovery rebases the captured pose into the checked capsule frame and blends
for 0.35 seconds before playing the complete authored get-up. It writes no
health and cannot complete after the actor enters death.

Death preserves the evaluated animated/physical pose, absent pieces and the
existing corpse/blood/debris budgets. One authoritative live-set removal
governs death eligibility. Pickup origin follows the physical pelvis when the
disabled transport capsule no longer matches the visible body.

## Queries and controls

LMB is the only fire input. RMB selects head height, Left Ctrl selects low
height, and no modifier selects torso height; Ctrl has priority. Those planes
remain 168/65/125 cm above the player's floor. They do not follow anatomy.

Regional capsule components follow evaluated bones through animation,
upper-body physics, full falls, recovery and death. The weapon segment wrapper
merges the normal world Visibility hit with exact capsule intersections. It
uses the same ray, scaled dimensions, visibility response and ignore filters;
an existing world hit wins a distance tie. This corrects a reproduced Chaos
cylinder-middle numerical miss without enlarging geometry or redirecting aim.
Removed regions remain excluded even if a stale primitive were re-enabled.
The existing per-projectile ignored-actor and per-discharge victim transaction
rules prevent duplicate damage from overlapping regions or physical bodies.

The motion fixture reproduced ten real Ctrl+LMB shots passing above a corpse:
the selected world plane was Z 68.474 cm and its body-query top remained
61.743–62.698 cm. Floor-level anatomy can remain below the fixed low plane.
A direct regional query is not proof that the ordinary mouse controls can
place a shot on that region. This limitation is retained and disclosed.

## Audio and verification

[Recorded-audio integration](Candidate07Audio.md) documents sources, real
operation/contact events, voice limits and cleanup. Digital checks and cue
counters are separate from perceptual audition.

`ONE07PhysicalityCheck` is a disclosed sandbox fixture with setup, explicit
fall/sever probes and observation health restores. `ONE07Encounter` sends only
production movement/fire/reload input through an ordinary encounter.
`ONE07PortabilityCheck` uses the existing Portability06 map and requires all
three appearances. None is human native play. Profile runs disable recording,
retain every measured frame and record actual body/voice/state occupancy.
Final source, package, media and public-download gates remain separate.
