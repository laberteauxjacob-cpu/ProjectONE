# Final packaged audio measurements

Source: `6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Both recordings contain the actual engine master mix at 48 kHz, stereo, 16-bit PCM before movie AAC encoding. The complete measured data is in [motion_signal.json](motion_signal.json) and [presentation_signal.json](presentation_signal.json).

| Recording | Duration | Sample peak | RMS | Full-scale samples |
| --- | ---: | ---: | ---: | ---: |
| Motion | 51.178667 s | -15.052428 dBFS | -46.498343 dBFS | 0 |
| Presentation | 196.949333 s | -14.795334 dBFS | -41.079295 dBFS | 0 |

The motion signal/counter audit passes its declared checks across 23 phases: four attack-cue increases, three hit-cue increases, one death-cue increase and six committed shots. The six individually observed shot outcomes are four LiveHit, one NewKill and one CorpseHit. Mixed signal exists in the declared event neighborhoods. Cue counters establish accepted gameplay notifications, not perceptual audibility of an isolated voice.

The presentation audit deliberately retains **FAIL** for its generic requirement that every phase have samples above -60 dBFS. Phases 92 and 93 are the paused scene (0.180438 and 2.487396 seconds); phase 96 is the declared fatal-damage/game-over fixture (4.354771 seconds). Those three measured windows contain zero nonzero samples. Their original chapter labels identify pause and death, and the death lifecycle explicitly shuts down room and zombie audio. This is consistent with the captured lifecycle states, not evidence of a missing gameplay soundtrack. The original generic failures are not removed or reclassified as an all-phase audio pass.

The presentation mix has measurable signal elsewhere, including the quiet facility and actual base/upgraded weapon sequences. No sample reaches full scale in either recording. These are sample-domain measurements, not a true-peak, loudness, synchronization or perceptual mix approval. Frame callback times bound event neighborhoods; a nonzero mixed signal cannot identify one particular cue.

**No perceptual listening was performed.** Two capability attempts, including September 9, reported that audio input could not be delivered to the reviewer. The second probe used an earlier development mix and is not final-source evidence. The released films retain real game audio for human review of timbre, level, localization, variation and base/upgraded identity.
