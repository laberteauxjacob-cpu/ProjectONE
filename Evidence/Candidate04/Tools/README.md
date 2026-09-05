# Candidate04 evidence assembly

`assemble_candidate04_evidence.py` is an evidence-only adaptation of the assembler in Scripts. It leaves the built game, source capture, frames.csv, checks.txt and original Unreal master-output WAV unchanged. Its default behavior rejects a final callback at or beyond the WAV end, as the original assembler does.

The explicit `--clip-terminal-idle-to-audio` option handles a narrow recorder shutdown overlap: omit only callbacks whose original audio-clock timestamp is at or after the actual PCM WAV duration. It requires all of these conditions:

- At most three omitted callbacks; the last is no more than 0.1 seconds past the audio end.
- Every omitted callback belongs to the final original chapter labeled exactly `FINAL INVENTORY / BOTH MACHINES RETURNED TO IDLE`.
- The final phase retains at least two seconds, including at least two seconds between its actual retained callbacks. No firing, reload, machine transition or entire phase is removed.
- The last retained frame's hold to the real audio end remains within the original half-second bound.
- Original completion is successful and the unmodified raw check count equals the complete original frames.csv count.

All original JPEG headers/dimensions and hashes are checked, including the omitted callbacks. The original ordered frame identity, original CSV/WAV/check/chapter hashes and original captured count remain in the sidecar. It separately records the presented count and identity, the original and presented final timestamps, exact audio end/hold, and each omitted original filename, timestamp, index, phase and JPEG SHA256. The `frames` field continues to mean original captured callbacks, not presented callbacks or encoded CFR frames.

The presented sequence ends at the real WAV duration. There is no audio padding, stretching, gain, replacement or retiming. The normal assembler samples callback holds onto a 30fps CFR grid; it does not generate motion between images. Its first-image hold and ordinary within-capture holds are unchanged. This bounded idle-tail omission is explicit in `terminal_idle_clip`, not a silent repair or altered source capture.

Run only after capture and profiling stop, using the actual final capture folder, exact built-source SHA and intended new output:

```powershell
py -3 Evidence/Candidate04/Tools/assemble_candidate04_evidence.py --input <capture-folder> --output <progression_loop.mp4> --capture-kind packaged --source-state exact-commit --source-revision <built-source-full-SHA> --chapters --clip-terminal-idle-to-audio --validate-only
```

Remove `--validate-only` and supply the installed FFmpeg with `--ffmpeg <ffmpeg-executable>` to encode. Validation reads and hashes all actual original inputs; it is substantial work and must not run during performance measurement. Output and its sidecar are refused if they already exist unless the explicit intended-Candidate04 `--replace` option is supplied.

The concise comparison assembler must use the same explicit option and verify this complete original/presented ledger before selecting earlier fire/reload and dim-machine excerpts. Counter and integrity checks remain separate from visual inspection, perceptual audio listening and native-input success.

Run the chapter metadata postprocess after encoding the full movie and before encoding its comparison:

```powershell
py -3 Evidence/Candidate04/Tools/repair_movie_chapters.py --movie Evidence/Candidate04/progression_loop.mp4 --commit <built-source-full-SHA> --expected-chapters 80 --ffmpeg <ffmpeg-executable>
```

`repair_movie_chapters.py` accepts only `progression_loop.mp4` and `weapon_comparison.mp4` directly under `Evidence/Candidate04`. It verifies the current movie against its encoded sidecar and the exact source SHA, then preserves the original movie and sidecar under ignored `Saved/Candidate04/Assembly`. It rebuilds chapter metadata from the full sidecar's `chapters`, or from the comparison's `clips` using their output start/end times. All intervals must be contiguous and cover the expected output duration.

The stream-copy remux restores explicit chapter titles and valid chapter-track references without re-encoding video or audio. Before replacement it requires identical encoded AV stream payload hashes, matching chapter titles/times within MP4 millisecond rounding, and a successful full AV decode without the missing-QT-track warning. The sidecar records the new file SHA256/size, original file SHA256, helper SHA256 and verification results. This does not change original capture frames, timestamps, WAV audio, the idle-tail selection, or the gameplay source. Numerical decoded audio measurements are not a listening claim.

The comparison must be encoded from the repaired full movie and its updated sidecar so its source hashes bind the published full movie. If comparison chapter readback still has blank titles or a missing-QT-track warning, run the same helper explicitly on `weapon_comparison.mp4` after comparison encoding, using its actual expected clip count. Never repair or replace a full movie while another process is using it to encode a comparison.

Tiny independent fixtures (no real capture/media or engine work):

```powershell
py -3 Evidence/Candidate04/Tools/test_terminal_idle_policy.py
```
