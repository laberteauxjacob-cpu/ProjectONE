# Candidate07 — infected physicality and recorded audio

Status: internal checkpoint A, baseline/source audit. Candidate07 is in progress;
it has not been built, visually accepted, packaged or published. The latest
verified playable release remains Candidate06.

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
Interpretation and comparison are pending. A separate actual-engine motion
capture completed 80 checks with one failure: its later low corpse trace missed.
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

## Completion gates still pending

First character in gameplay; connected attack and physical-contact checkpoints;
living fall/recovery and lethal-state checks; all-variant dismemberment/query
fit; recorded audio and event synchronization; chronological engine review;
representative comparative performance; full regression checks; fresh neutral
build, source/LFS verification, archive and public download verification.

Candidate06 controls remain: LMB fires; hold RMB for head height; hold Left Ctrl
for low height; release both for torso height. Ctrl takes precedence when both
height modifiers are held. No height modifier fires a weapon.
