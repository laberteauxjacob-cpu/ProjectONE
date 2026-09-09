# Candidate05 S4 final movie verification

Both current movies match their encode reports and completed full-decode records. Source: `6b8621cef9d2a87de6e6eabc1743359c6274da5a`. This summary covers file identity and media structure; visual reviews, audio audition and performance profiles remain separate.

| Movie | Original callbacks | Encoded / fully decoded video frames | Video sample duration | MP4 container duration | Original WAV duration |
|---|---:|---:|---:|---:|---:|
| motion | 1389 | 1523 | 50.766667s | 51.179000s | 51.178667s |
| presentation | 5257 | 5897 | 196.566667s | 196.950000s | 196.949333s |

Both streams are H.264 High video at **1600×900, constant30fps**, with **AAC LC48kHz stereo**. There are no embedded chapters; the original chapter CSVs remain bound capture records. Encode sidecars and completed assembly logs record exit0. Root observed exit0 for both full FFmpeg decodes, and their terminal frame counts agree with the actual MP4 sample counts. The logs contain video/audio completion summaries and no error terms. Those numeric decode exit codes came from root's process observation, not from the text logs alone.

The source capture-to-WAV final holds are **0.413608s (motion)** and **0.402170s (presentation)**, both within the source assembler's positive≤0.5s policy. The encoded video tracks end at50.767s and196.567s, while their containers end at51.179s and196.950s. Approximately0.412s and0.383s of audio/container time remain after the video tracks. A player may keep the last image visible during this remainder, but that implicit display was not verified through continuous playback here; no additional encoded terminal frames are claimed.

AAC raw media durations are51.200s and196.970667s, including1024 priming samples. The edit lists start at sample1024 and have durations51.178s and196.949s. Movie track/container/edit times use a1000Hz timebase, so small sub-millisecond differences from the exact recorded WAV endpoints are disclosed rather than treating all durations as identical.

The timestamped source callback rates were27.350196/s and26.743853/s. Resampling onto the30fps output grid yields net frame-count increases of134 and640. These are **net increases**, not measured individual duplicate counts; warning-level encode logs do not record the separate duplicate/drop totals. Duplication or dropping on the fixed output grid is expected. The assembler uses held original JPEGs and the recorded engine master WAV, without motion interpolation or synthesized replacement audio. The game's own sound assets may be provisional/synthesized.

Current movie, sidecar, source CSV/check/WAV and log hashes were checked. All source-frame ledger entries match the completed capture runner, and both ordered ledger digests were recomputed successfully. Actual WAV headers confirm stereo16-bit PCM at48kHz. MP4 atom parsing independently confirms video sample counts/timing and audio structure. Source JPEG payloads were not all rehashed again for this summary. No encoder, decoder, UE, build or Git process was launched by this verification.

Both encodes logged a stereo-layout guess and a deprecated pixel-format notice; actual WAV/MP4 metadata confirm stereo, and the decode logs report full-range yuvj420p. No perceptual color or audio quality claim follows from successful encoding/decoding.

**No continuous-playback review or perceptual audio audition is claimed.** Readable codecs and valid file hashes do not replace the separate gameplay, visual, audio or performance evidence.

| Artifact | Bytes | SHA256 |
|---|---:|---|
| Releases/Candidate05/motion_review.mp4 | 15982534 | `e92e0d8bfc15dde401609d5e9b41bcfc4351a6e07b8b7f7304856928b4afdaed` |
| Releases/Candidate05/motion_review.json | 288841 | `26f23112030664e7a961a851373beb015dd4459a7e4504821069b294c1f72e6c` |
| Releases/Candidate05/presentation_review.mp4 | 41201372 | `a9a6b381d14231d025272a3365ddf25a2e1d41a9ee0bc4bd733e29eb3b9bdcf1` |
| Releases/Candidate05/presentation_review.json | 1073441 | `3325cc8487a82f76654e32c6b8e9a7deac3d678a6421694187fc2b948cdfc82e` |
