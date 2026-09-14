# Candidate07 — infected physicality and recorded audio

Status: internal checkpoint B, first Maintenance character integrated and
inspected in actual Editor-game frames. Candidate07 remains in progress; it is
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

Connected attack and physical-contact checkpoints;
living fall/recovery and lethal-state checks; all-variant dismemberment/query
fit; recorded audio and event synchronization; chronological engine review;
representative comparative performance; full regression checks; fresh neutral
build, source/LFS verification, archive and public download verification.

Candidate06 controls remain: LMB fires; hold RMB for head height; hold Left Ctrl
for low height; release both for torso height. Ctrl takes precedence when both
height modifiers are held. No height modifier fires a weapon.
