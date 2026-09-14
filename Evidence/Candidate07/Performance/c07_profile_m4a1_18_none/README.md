# Candidate07 profile

M4A1; requested live 18; explicit falls none; packaged.

Source commit: 3d75c2faa0cecef6075f00755d8e4f5dd2562817. Observed HEAD: 3d75c2faa0cecef6075f00755d8e4f5dd2562817. Input snapshot: 24349e0ea442580295f6b08fbc5c29421403d22d25a01ea058d11b162d72af33.

All 3023 engine rows retained, including 0 zero-duration rows. Frame time mean 10.1898 ms; p95 12.4931; p99 15.1796; max 46.2420.

Legacy driver exact-population fraction: 0.624752; both machines active 3.215600 profile seconds. Requested weapon committed 144 shots.

C07 state occupancy and all per-frame body/voice counters are recorded in [analysis.json](analysis.json) and [census.csv](census.csv).

[Complete numeric engine timeline](engine_timeline.csv). Raw evidence hashes, host metadata and scope timings are in [analysis.json](analysis.json).

- Every engine timing row, including zero frame 0, is retained. Percentiles interpolate at (n-1)*p; timings are milliseconds.
- Missing early numeric cells are zero per UE stream semantics. Raw event text and private metadata remain in hash-bound originals.
- Duplicate non-identity series labels retain every positional column with its zero-based raw column index; the original final header and full mapping are included.
- Nested and worker scope durations are not summed into wall time. Voice counts mean component playback, not mixer audibility.
- C07 post-driver census includes replenishment; legacy driver live counts are sampled before its replenishment and are retained separately.
- Actor contact/fall/recovery sums are current-actor snapshots and can decrease after retirement.
- Simulated/awake body census covers infected leader meshes; detached debris and other world physics are represented only by the separate engine counters.
- C06 supports unchanged 6/12/18 profiles. C07 1/2 and explicit mixed falls are additional workloads, not manufactured matched baselines.
- One scripted, warm, capped run with disclosed health restoration, machine/pickup setup and unspecified random streams; no universal FPS, native-input or audio-review claim.
