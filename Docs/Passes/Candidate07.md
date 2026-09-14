# Candidate07 — infected physicality and recorded audio

Status: internal checkpoints D/E, physical contact, all three appearances and
recorded event audio integrated in the Editor game. Candidate07 remains in progress; it is
not packaged, publicly released or approved by the user for visual/audio quality.
The latest verified playable release remains Candidate06.

## Preserved baseline

- Published source/documentation revision: `90ead259e10ef7ad7ed8803b0236aa02bb75eac6`.
- Candidate06 gameplay build source: `c2c422012f4f219fb5e63ac24294a6ba8428f999`.
- Candidate06 release archive SHA256: `4dd954cdeb894d2be371fa78df0f640ef7a623685c40aa5cf0261c804f1c8b0a`.
- Existing candidate branches, tags, source and packages are preserved. Work
  continues on `codex/candidate07`; Project Zero is excluded.

Preflight on 2026-09-14 UTC verified UE 5.7.2 CL49658320 and Blender 5.1.2.
Available disk was approximately 871 GiB; the initial incremental allowance is
10–20 GiB for bounded authoring, capture and one fresh neutral release build.
No old candidates or unrelated files were removed.

The extracted Candidate06 runtime matched all six published runtime hashes.
Native launch, ordinary game-over, Enter restart to 100 health / M1911 7+56,
and Q quit were inspected. No security prompt was approved or bypassed.
Supported review includes discrete native input, screenshots, actual engine
frame/audio capture and offline decode. Held native input and perceptual audio
audition are not established. Engine master-output recording is distinct from
listening; microphone input is not used.

One new baseline M4A1/six-enemy performance scenario completed all 38 checks.
All 2,185 frames were retained: mean 14.1072 ms, p95 22.7607 ms, p99 24.2811 ms,
worst 34.9726 ms. Exact six-enemy occupancy was 81.41% of registered counter
frames; comparison remains pending. A separate actual-engine motion capture
completed 80 checks with one failure: its later corpse trace missed. Inspection
found that the historical harness did not actually hold Ctrl for that shot;
it does not establish a failure of a genuine 65 cm low-aim input.
The failure and raw capture are retained for compatibility analysis; this is
not reported as a passing baseline motion validation. An unrelated game was
already running on the host, so these runs do not establish an isolated host.

## Audit and implementation direction

The actual C03 infected has five modular meshes, 21 source bones and the same
rig plus imported armature node. Imported anatomical left uses `_r` suffixes.
Source arm, thigh and head cut boundaries must remain coherent under evaluated
poses. The player uses an independent bind and remains outside the art change.

The shared C05 animation graph explicitly samples sequences, so imported
notifies cannot be presumed to play. The infected's chest-height wrist targets
are authored into the clips. Attack selection currently cycles families, entry
discards approach speed, and recovery explicitly stops movement. Capsule/RVO
avoidance alone is not physical upper-body response. Those paths are being
replaced in an infected-specific animation/attack/contact implementation.

The first new Maintenance model must pass source and in-game inspection before
Laboratory and Facility Staff appearances are multiplied. Appearance data has
no health, speed, damage or reward settings. Living falls remain the same live,
damageable actor and never award kills or drops. Recovery requires local floor
support and clearance; dead actors cannot resume recovery.

Recorded source research began during preflight. Actual firearm recordings,
performed creature vocalizations and physical Foley have been located with
asset-specific license evidence. Only selected sources with both commercial
and public raw-source redistribution rights will be integrated. Exact analog
weapon provenance, compressed-preview sources, attribution, hashes and edits
will be recorded separately. No synthesized physical cue will be described as
a recording. Existing upgraded energy and useful machine/pickup accents stay.

## First integrated character checkpoint

Maintenance now uses five original modular meshes, 60,406 source triangles,
modeled facial damage, partial hair, thick torn/stained workwear and authored
vertex color/roughness with small original normal maps. The imported skeleton
and player assets retain their existing contracts. All 140 paired source seam
samples match. The measured skull query uses center (-0.2, 0, 168.7) cm,
radius 9 cm and half-height 12.3 cm; the interior physical head uses radius
8.5 cm and a 6.6 cm cylinder. Other existing body/constraint parameters remain.
This is a mesh-fit correction, not an aiming-height or target-selection change.

The infected-specific graph samples fourteen editable motions. Revised prone
and supine clips are 2.55 and 3.0 seconds, preceded by a 0.35-second captured-pose
blend. The saved Blender source was reopened to verify all fourteen actions;
twelve original motion FBXs remain byte-identical. The current supine entry
still needs a grounded torso/hip roll correction. The prone gameplay sequence
shows palm brace, knee tuck, foot placement and rise; its short initial pose
blend remains visibly procedural. This is bounded chronological frame review,
not continuous playback or a claim that all motion is finished.

Hybrid upper-body simulation, living fall/get-up states, event-based recorded
audio and an exact regional capsule query are integrated. The regional query
addresses a reproduced Chaos cylinder-middle ray miss using the unchanged ray
and current capsule dimensions; world occlusion, near misses, ignored actors,
absent limbs and per-victim transactions are tested. A physical death's pickup
origin now follows the evaluated pelvis instead of an abandoned capsule.

The second full engine automation run executed 37 tests: 36 passed (three with
warnings), while one catalog test failed eighteen obsolete Candidate05 audio
path expectations. That expectation was deliberately migrated to the exact
Candidate07 wave banks; its separate follow-up passed. A complete fresh-build
suite remains required. Original failures and diagnostics are retained privately.

Two complete WIP viewport/master-audio captures have been produced. The latest
80.49-second capture has 2,151 actual frames and 76 assertions with one failure:
runtime obstacle mesh assignment was rejected. The fixture setup is corrected
in subsequent source but has not yet passed a new recording. This capture is
REVIEW_ONLY, with its actual compiled DLL identity recorded separately. It
does not prove obstacle contact or a finished crowd system. The first character
was reviewed at the ordinary camera and through its modular death sequence;
appearance remains subject to user judgment. No perceptual audio audition occurred.

The source audio inventory now contains 76 originals and 134 processed cues.
Exact rights, analogues, retained synthetic energy layers and attribution are
documented in Candidate07Audio.md and the source inventory. Library verification,
event checks and digital clipping analysis do not substitute for listening.

## Completion gates still pending

Final combined attack/physicality/audio review and ordinary recovery-delay
investigation; representative comparative performance; full
packaged regression checks; fresh neutral
build, source/LFS verification, archive and public download verification.

Candidate06 controls remain: LMB fires; hold RMB for head height; hold Left Ctrl
for low height; release both for torso height. Ctrl takes precedence when both
height modifiers are held. No height modifier fires a weapon.

## Connected-combat checkpoint

The actual Editor-game motion recording completed 90 assertions with one
retained compatibility failure. All three attack families entered through the
production selector, delivered one 19-health contact and one effort cue, and
returned to pursuit. Forty recorded frame samples were inspected, including
twelve chronological poses per family and the same-scale Candidate06/Candidate07
camera comparison. Swipe, cross-body rake and two-handed forward strike have
different windups/follow-through; the camera can occlude the far hand. This is
bounded frame review, not uninterrupted human play or complete naturalism approval.

The historical isolated fixture spawned its enemy facing away. Candidate07's
bearing eligibility correctly rejected that setup; the fixture now starts
facing the player and logs distance, bearing and grounded state. Production
selection/fairness was not relaxed. The failed earlier recording is retained.

The completed run reproduced a genuine fixed-height limitation: ten shots with
processed Left Ctrl and LMB selected Low at world Z 68.474 cm while the corpse's
body query top was 61.743–62.698 cm. All ten missed; the corpse assertion remains
a failure. These controls do not follow floor-level anatomy, and direct regional
query tests do not establish ordinary mouse targeting. No snapping, larger hit
regions or control redesign was added to hide this incompatibility.

The complete old/new engine WAVs were measured and concatenated without gain,
trimming or replacement for private listening comparison. The current WAV has
zero full-scale samples and zero near-clip windows. Actual PCM segments match
their sources; no sound was perceptually heard or approved. Candidate07 remains
an Editor WIP with null exact packaged-source identity, and final media/package
evidence remains required.

## Physical-contact, variants and audio checkpoint

The source trio is Maintenance (60,406 triangles), Laboratory (64,798) and
Facility Staff (64,982). Laboratory adds lined coat tails/lapels and grey
receding hair; Staff adds rolled sleeves, bare forearms, tan trousers and a
different crown/hair treatment. They share the rig, paired cuts, head-query
envelope and gameplay statistics. All ten new source previews were inspected;
coat/hip and hair-surface intersections found during authoring were corrected
before import. Both new imports completed with zero errors and warnings.

The latest physicality recording completed all seven phases: 84 assertions,
zero failures, 2,144 original frames and 80.448 seconds of actual engine audio.
It includes one/two/group contact, an obstacle, mixed living falls, both actual
recovery clips and a declared two-of-each appearance lineup. Prone and supine
recoveries retain 112 health, and the recorded snapshot rebases report 0 cm.
Bounded chronological review found a readable Maintenance floor roll and
supported rise; Staff's initial roll is partly occluded. Laboratory's coat
remains coherent in sampled contact and folded death poses. Hidden crowd
contacts/seams and continuous playback are outside this review.

The revised supine source uses measured garment/head/boot support and retains
the other thirteen FBXs. Its actual gameplay recovery lasts 3.35 seconds,
including the initial 0.35-second snapshot blend; prone lasts 2.90 seconds.
Both still have a brisk, stylized initial transition.

Subsequent integration review fixed two concrete issues: non-solid pickup
collection spheres no longer supply floor support or block recovery, and
stopped attack footplants can use grounded movement from the same action's
preceding 0.22 seconds. Idle/airborne/stale callbacks remain rejected.
That checkpoint's 38 engine tests passed (35 clean, three with warnings), including real
pickup availability through recovery and the stopped-footplant regression.
The 80.448-second recording predates these two fixes; final packaged recordings
are still required. The combined build also exposed and fixed a Unity-build
test-helper name collision.

That recording encoded to a 15.5 MB private preview and passed 31 integrity,
timing and complete video/audio decode checks. It remains REVIEW_ONLY with
null exact packaged-source identity. No perceptual audition occurred. Current
source inventory records 459 C07 LFS payloads; visible-string review found no
actionable private-path/token matches. Final outgoing-history, fresh checkout,
package and publication audits remain separate.

## Combined-review preflight

The initial ordinary encounter completed with all three appearances: 24 actual
actors (nine Maintenance, nine Laboratory, six FacilityStaff), 16 kills and
eight remaining living enemies. Player health reached 81 and regenerated to
100; the driver performed no health restoration. This is automated WASD,
cursor selection and LMB input, not native free play. Its late 0/0-ammunition
state caused useless repeated reload requests; the fixture now requests reload
only while reserve ammunition is available. Bounded review also found two
living fallen bodies waiting over 32 seconds for recovery clearance. That
delay led to the bounded physical recovery effort described below.

The three-appearance portability run passed 150 assertions, including actual
navigation/attacks, a disclosed fall and damage packets, local recovery with a
missing arm, death/scoring and cleanup for each appearance. It recorded 823
frames. The six-weapon combat run passed 2,119 assertions on the new bodies.
These are Editor-game preflight checks, not final packaged evidence.

The first recording-free M4A1/12/mixed-fall profile retained all 3,044 engine
rows: mean 10.1242 ms, p95 11.8822 ms, p99 13.0582 ms, maximum 295.541 ms.
Seven frames exceeded 16.7 ms and two exceeded 33.3 ms. The analyzer initially
rejected duplicate Unreal texture-streaming column names; it now retains both
positional series and the original header. No timing rows or spikes were
removed. This single Editor WIP sample is not the final same-host packaged
comparison or a universal performance claim.

The follow-up recovery regression now passes with actual imported bodies: an
intact living body reaches supported frozen rest, resumes that pose, physically
moves clear of a still-present corpse through three finite efforts, and begins
get-up at unchanged 112 health. The final interval reports 65.902 cm accumulated
path and 48.894 cm/s maximum pelvis speed. A real arm-sever packet during get-up
remains absent through completion and a second fall. Static cover and missing
floor reject the effort. Setup uses a declared frozen-corpse placement and
temporary solver sleep suppression before the trial; this is an engine fixture,
not ordinary player input or visual approval. Earlier prerequisite failures are
retained: a missing-arm pose did not satisfy the unchanged two-degree rest
window. The existing separate missing-arm recovery check is unchanged.

The complete current 39-test engine suite then passed: 36 clean, three with
warnings, zero failures or skipped tests. Fresh public-source packaging and
the final recorded/uncaptured runtime matrix still remain separate gates.
