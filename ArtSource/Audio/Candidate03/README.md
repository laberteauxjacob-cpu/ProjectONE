# Candidate03 original shot bank

Twelve locally synthesized mono PCM16 WAVs, 48 kHz. The editable source is
[create_candidate03_audio.py](../../../Scripts/create_candidate03_audio.py);
[manifest.json](manifest.json) records every layer profile, SHA-256, envelope,
band measurement and exported-file level. No recordings, sample libraries,
external services or Project Zero content are used. Candidate02 audio remains
unchanged.

## Intended identity and variation

The carbine is designed around a brief high-frequency crack, compact mid-band
pressure, short staggered cycling contacts and a restrained reflected tail.
The shotgun uses an irregular multi-pulse front, broader granular body, a short
receiver response and a longer diffuse decay. These are intended distinctions;
realistic timbre and mix clarity have not been perceptually approved.

Each family's six profiles change attack filtering, body bandwidth and decay,
microburst timing, action-contact delay, reflection spacing and tail filtering,
as well as independent noise seeds. They are not six copies with only a pitch
change. The complete source lasts 0.42 seconds for each carbine variant and
0.66 seconds for each shotgun variant. The shotgun shot contains no delayed
pump cycle: the existing separate pump, extraction and lock events remain tied
to the actual weapon operation.

| Files | Peak dBFS | Whole-file RMS dBFS | Tail RMS after 95 ms dBFS |
| --- | ---: | ---: | ---: |
| S_C03_CarbineShot_01 | -5.00 | -28.60 | -61.21 |
| S_C03_CarbineShot_02 | -5.00 | -29.04 | -61.95 |
| S_C03_CarbineShot_03 | -5.00 | -29.87 | -57.84 |
| S_C03_CarbineShot_04 | -5.00 | -29.55 | -63.77 |
| S_C03_CarbineShot_05 | -5.00 | -28.50 | -58.94 |
| S_C03_CarbineShot_06 | -5.00 | -29.32 | -59.99 |
| S_C03_ShotgunShot_01 | -4.50 | -26.21 | -39.28 |
| S_C03_ShotgunShot_02 | -4.50 | -28.58 | -42.49 |
| S_C03_ShotgunShot_03 | -4.50 | -26.56 | -38.49 |
| S_C03_ShotgunShot_04 | -4.50 | -27.31 | -41.67 |
| S_C03_ShotgunShot_05 | -4.50 | -27.49 | -40.54 |
| S_C03_ShotgunShot_06 | -4.50 | -28.57 | -41.89 |

All twelve exports have zero full-scale samples, absolute DC offset below
0.0001 and distinct SHA-256 values. The table measures quantized source PCM,
not the final engine mix. Long quiet tails lower whole-file RMS; these numbers
must not be treated as perceived loudness ratings.

## Import and runtime contract

Generate with `python Scripts/create_candidate03_audio.py`. Verify without
rewriting with `python Scripts/create_candidate03_audio.py --check`.
The targeted importer is
[import_candidate03_audio.py](../../../Scripts/import_candidate03_audio.py).
Run it in the project's coordinated Unreal commandlet process with
`-AllowCommandletAudio`; that flag is required for the BINKA decoder.
It imports exactly these twelve SoundWaves under
`/Game/ONE/Audio/Weapons/Candidate03` and verifies duration/channels/source hash.
Source generation/checking does not imply that Unreal import has occurred.

Connect all six `S_C03_CarbineShot_01` through `_06` and all six
`S_C03_ShotgunShot_01` through `_06` to the respective editable weapon bank.
Avoid immediately repeating the previous candidate. Compare at pitch 1.0 so
the actual authored differences can be judged. Existing Candidate02 magazine,
bolt, pump, shell-insert, equip and distinct empty events retain their existing
timings and source files; this shot-bank pass does not claim their redesign.

## Review status

Local generation and deterministic source verification passed. No available
tool in this session provides perceptual review of non-speech weapon audio;
speech summarization is not a substitute. Sound identity, realism, subtle
variation and busy-combat mix quality remain provisional until heard. The
coordinated gameplay comparison must use genuine engine master audio, check
that both weapon phases are audible, and provide the user a listening result.

For the sixteen-phase Stage C capture, run
`python Scripts/audit_candidate03_presentation_audio.py --source Saved/Candidate03/PresentationCapture/gameplay_master.wav --frames Saved/Candidate03/PresentationCapture/frames.csv --observations Saved/Candidate03/PresentationCapture/observations.csv`.
The default report is `Evidence/Candidate03/StageC/audio_metrics.json`.
It requires all sixteen annotated phases, sufficient recorded duration,
non-silent firing and actual reload windows, no full-scale PCM samples and a
matching twelve-file source bank. Phase timing follows `audio_seconds` in the
actual frame manifest. Optional shot-ID/index telemetry checks real selected
indices for repeats; waveform bursts are never substituted for dispatch counts.
No Candidate02 evidence is rewritten.
