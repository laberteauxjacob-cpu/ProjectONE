# Three Candidate06 pickup cues

`Scripts/create_candidate06_pickup_audio.py` authors exactly three original,
intentional stylized electronic rewards. Insta-Kill uses a descending paired
pulse, Double Points two ascending note pairs, and Max Ammo three stepped
latches with a refill note. These are synthesized designs, not recordings.
No weapon, mechanism, footstep, creature or environment bank is replaced.

The deterministic standard-library generator reuses Project ONE's own signal
utilities and writes mono 48 kHz, 16-bit PCM WAVs (0.86/0.74/0.88 seconds) plus
a manifest of hashes, seeds, durations and measured signal statistics. Run
`py -3 Scripts/create_candidate06_pickup_audio.py` to generate; `--check` verifies
byte identity without writes. Root schedules generation and targeted Unreal
`import_candidate06_pickup_audio.py` separately from other engine work. No
external sample, recording, purchase or Project Zero input is used; WAVs carry
only PCM format/data metadata.

Each collection requests one cue at gain 0.7 through the existing bounded Action
concurrency group. The actor retains its voice briefly; reset/destruction stops
it. Event counters and valid PCM do not establish that every voice was audible,
perceived timbre, spatial balance or player approval. Import, engine recordings,
numerical measurements and any perceptual listening remain separate evidence.
