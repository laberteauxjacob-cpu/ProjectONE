# Candidate06 recordings

Package source: `c2c422012f4f219fb5e63ac24294a6ba8428f999`. All four final films passed 24 format, source-binding and full-decode checks each. The reports below bind the actual MP4 bytes, original captures and six package runtime identities. Public download verification is separate from this local verification.

| Film | Source JPEGs | Decoded MP4 frames | Original WAV seconds | Source peak / RMS dBFS |
| --- | --- | --- | --- | --- |
| [Combat](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_combat.mp4) | 2,095 | 2,374 | 79.125333 | -12.405 / -38.294 |
| [Grouping](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_grouping.mp4) | 1,135 | 1,325 | 44.181333 | -12.990 / -38.399 |
| [Survival](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_survival.mp4) | 1,735 | 2,001 | 66.688000 | -16.500 / -45.344 |
| [Portability](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate06/candidate06_portability.mp4) | 1,131 | 1,345 | 44.842667 | -17.747 / -40.915 |

All outputs are H.264 1600x900 at 30 fps with AAC 48 kHz stereo. Actual pixel format is `yuvj420p`; PyAV reports JPEG/full colour range (value 2). Source image counts differ from output frame counts because the original timestamped stills are sampled at 30 fps. [manifest.json](manifest.json) supplies exact hashes, durations, frame holds, colour metadata and report inventory.

Per-film verification and original-PCM measurements: [combat](combat/inspection.json) / [audio](combat/source_audio_metrics.json), [grouping](grouping/inspection.json) / [audio](grouping/source_audio_metrics.json), [survival](survival/inspection.json) / [audio](survival/source_audio_metrics.json), [portability](portability/inspection.json) / [audio](portability/source_audio_metrics.json).

Assembly uses actual engine viewport JPEG callbacks and the original master WAV. The first image is held from audio zero, subsequent images follow their recorded callback times, and the last actual image is held to the WAV endpoint. An explicit cloned final-still tail plus 30 fps sampling implements that hold; output duration remains bounded by the unchanged WAV. No motion interpolation, invented images, audio replacement, tempo change or synthetic silence is used. Codec endpoint quantization and the unmeasured CPU-callback/audio-render start offset are disclosed in each inspection.

The exact assembler used is `assemble_candidate06_capture.py`, SHA-256 `ebc3259e83cbe0886a42c61ee819d04b7c26ca926e18ddf250d0187baa1ef96a`. It is an evidence-production correction after the packaged source commit; it changes no game runtime. The first combat encode was rejected before decode: its video ended early and the verifier assumed the limited-range pixel-format name. That MP4 and failure report remain private. The corrected four outputs were independently probed and fully decoded to completion.

All four original PCM measurements contain zero full-scale samples. These are sample peaks/RMS, not true-peak or perceived loudness measurements. Every film includes a short measured quiet startup window; Survival also has quiet windows around the declared pause/death sequence. Silence and phase energy are reported with their timing limits, without an automatic audible-phase pass claim. No perceptual audio audition, timbre/localization approval or continuous playback is claimed.

These are controlled engine fixtures, with disclosed teleports, grants, frozen targets and simulated input where used. They are separate from native-input evidence, unrestricted human play, user visual approval and the six profiles captured without screenshot/audio recording or encoding.
