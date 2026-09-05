# Candidate03 review evidence

The [pass report](../../Docs/Passes/Candidate03.md) separates implementation,
automated checks, motion inspection, engine audio and packaged input. Earlier
working-tree recordings remain useful evidence, with their actual provenance;
they are not relabeled as recordings of the final package.

## Recordings

| Recording | Contents | Provenance |
| --- | --- | --- |
| [Directional movement](stage_b_directions.mp4) | Both weapons; eight directions; walk, sprint and reload movement | Stage B editor-game working tree; 48 labeled segments |
| [Stationary turns](stage_b_turns.mp4) | Left/right stepping turns and quick reversals, both weapons | Corrected Stage B editor-game working tree; six labeled segments |
| [Weapon presentation](stage_c_presentation.mp4) | Bright/dim firing; translation and rotation; both weapons; cases; repeated-shot variation | Stage C editor-game working tree; 16 labeled segments; genuine engine master audio |
| [Physicality](stage_d_physicality.mp4) | Wounds, trails/pools, both arm paths, head/leg loss, varied deaths, rest/resume, corner/rack/bench crowds and cleanup | Final S2 packaged source; 11 labeled chapters; genuine engine audio; 249-check capture |
| [Native controls](native_controls.mp4) | Actual native mouse/keyboard weapon selection, four shots, reloads, lighting, pause, one infected and death-input rejection | Final S2 packaged observer; derived navigation chapters; genuine engine audio; no scripted input or native movement claim |

Each movie has an adjacent JSON record containing source state, source-frame
timing, engine-audio duration, chapter times, encoding details and output hash.
Constant-frame-rate encoding holds or drops real captured frames according to
their measured audio clock. It does not invent interpolated motion. Any excluded
end callback is disclosed in that recording's record.

The audio is captured from the engine. Waveform and event measurements establish
output and variation, but no perceptual listening approval is claimed.

## Stage records

- [Stage B](StageB): measured movement, reload intent and bounded motion review.
- [Stage C](StageC): flash/material attachment, casing checks, actual shot events
  and measured engine audio. Full regenerable audio envelopes remain private.
- [Stage D](StageD): modular-source and physics inventories, repaired room
  collision/navigation, failed and corrected settling runs, blood and freeze/resume
  motion inspection. Earlier failed runs remain identified as failures.
- [Stage E](StageE): exact public-source build and runtime/archive audits, final
  [native input](StageE/native_input_review.md) and bounded motion reviews. The
  [first-source failure record](StageE/first_source_validation.md) preserves the
  stopped suite and the measured reason for correcting its legacy pursuit fixture.
- [Performance](Performance): preserved Candidate02 baseline and separately
  identified, completed Candidate03 measurements. Full captured spikes are
  retained; overlapping physics scopes are not added together.

Raw engine logs, personal diagnostics, debug symbols, dense source frames and
regenerable build output stay outside Git. Curated measurements and review
manifests retain enough provenance to distinguish what was actually inspected.

The byte-preserved `StageE/first_source_build.json` contains the original alias
`release_audit.json`. In that historical S1 record, this refers to
`StageE/first_source_release_audit.json`; the current `StageE/release_audit.json`
belongs to final source S2. The preserved S1 bytes have not been rewritten.
