# Candidate05 weapon timing

The 30, 60 and 120 FPS headless functional runs each passed **191/191 assertions**: 573 checks, zero recorded failures. This review read every assertion and all 17,671 timeline rows. The ordinary cadence sections had median game-frame intervals of 33.333, 16.667 and 8.334 ms, respectively. These are development integration results; they do not establish packaged, native OS input, rendered latency, audio quality or performance approval.

The [runtime probe](../../Source/ProjectONE/ONE05WeaponCheck.cpp) uses the actual six catalog variants, ammunition and mechanical events. Variant 2 is M4A1; variant 3 is Overcurrent. Stage 21 measures an established held burst, stage 31 drains a real full magazine, and stages 220–222 inject and observe a deliberate hitch. Shots are identified by increases in `shots`; discharge times come from `last_shot_time`, not the later observation row's timestamp. Restart resets the weapon counter, so its negative counter transition is excluded.

M4A1 is configured for 100 ms / 600 RPM. Overcurrent is configured for 100/1.15 = 86.956522 ms / 690 RPM. Measured full-magazine results follow; mean interval and RPM use the **N−1 intervals between N actual shots**, excluding reload time.

| Frame cap | Weapon | Shots | First-to-last span | Mean interval | Observed RPM | Gap lengths in frames | Maximum delay against ideal schedule |
|---:|---|---:|---:|---:|---:|---|---:|
| 30 | M4A1 | 24 | 2.300011 s | 100.0005 ms | 599.997 | 3 | 0.012 ms |
| 60 | M4A1 | 24 | 2.300080 s | 100.0035 ms | 599.979 | 6 | 0.080 ms |
| 120 | M4A1 | 24 | 2.300048 s | 100.0021 ms | 599.987 | 12 | 0.048 ms |
| 30 | Overcurrent | 36 | 3.066681 s | 87.6195 ms | 684.779 | 2 or 3 | 31.896 ms |
| 60 | Overcurrent | 36 | 3.050106 s | 87.1459 ms | 688.501 | 5 or 6 | 15.992 ms |
| 120 | Overcurrent | 36 | 3.050079 s | 87.1451 ms | 688.507 | 10 or 11 | 8.041 ms |

Overcurrent's finite-window RPM is not exactly 690 because each discharge occurs on an available game frame. Its delay relative to `first_shot + shot_index × 86.956522 ms` remained below one frame at every cap. Alternating adjacent frame intervals preserves the intended schedule instead of rounding every interval upward. The separate 1.8-second held-burst sections contained 18 M4A1 and 21 Overcurrent shots at every cap; their phase errors also remained below one frame. No observed frame increased the total shot counter by more than one.

Each automatic weapon received one deliberate 320 ms game-thread stall per run. The measured long frame was 321.382–321.906 ms. Each of the six cases produced exactly one recovery discharge. The following observation window contained zero additional shots before the deliberate controller key flush:

| Frame cap | M4A1 guard after recovery | Overcurrent guard after recovery | Recovery shots / catch-up shots |
|---:|---:|---:|---|
| 30 | 100.000 ms | 100.002 ms | 1 / 0 for each weapon |
| 60 | 83.332 ms | 83.336 ms | 1 / 0 for each weapon |
| 120 | 58.335 ms | 66.668 ms | 1 / 0 for each weapon |

This confirms no immediate catch-up burst in the recorded windows. The probe then intentionally flushes input; it does not record a long uninterrupted burst after the hitch. The production [deadline helper](../../Source/ProjectONE/ONEWeaponTiming.h) discards a whole missed interval and resumes from the actual discharge time.

All 18 eligible pre-weapon-tick taps fired in their request's engine frame, including same-frame press/release. All 18 eligible post-actor-tick taps fired at exactly the next pose dispatch. Their measured game-time delays were 33.330–33.335 ms at 30 FPS, 16.665–16.667 ms at 60 FPS and 8.333–8.335 ms at 120 FPS. CSV rounding accounts for microsecond differences. These are scheduling measurements, not mouse-to-photon latency: normal shots are observed by the probe on the following frame, and late shots on the second observation frame.

The passing checks also cover immediate rejection of equip/cooldown/pump/handoff presses, no delayed shotgun interrupt shot, committed magazine reload despite sprint/fire/switch and inventory requests, exact dry-click behavior, pause/key-flush disarming, actual death and OpenLevel restart. Loadout acquisition bypasses machine waiting only for declared setup. Input enters the production weapon component directly; the key-flush case uses the controller. Real scene collision/aim, mouse routing, audio and native held-input review require their separate evidence.

Source records are retained under the following portable run paths. SHA-256 identifies the exact inputs reviewed; the raw timelines are not reproduced here. Values are derived from stage 21/31 discharge rows and stage 220/221/222 hitch rows in each CSV.

| Record | SHA-256 |
|---|---|
| `Saved/Candidate05/Weapon30/checks.json` | `92cceea2b63cd22c41dd61d004e320a2258cb52b5198e0f08a03e2fe6b19cd08` |
| `Saved/Candidate05/Weapon30/timeline.csv` | `ff1a870dde7124e1b597c58d71b1f58bbec8fcfe53ae2fad4c0c8e2224666793` |
| `Saved/Candidate05/Weapon60/checks.json` | `64925a1185b52a20dfff333625003b04d6b4dd7dc6354a1a4cc1da8b4a2c04a7` |
| `Saved/Candidate05/Weapon60/timeline.csv` | `e604e69e4c8fa95f72e49b1a77d3e8990a3424a332634ba55b4e26df586de5be` |
| `Saved/Candidate05/Weapon120/checks.json` | `53eeb3310e25a90757f8564478242b5bf1d3bc90bdce7f44d2176e0d5c6bf440` |
| `Saved/Candidate05/Weapon120/timeline.csv` | `5c61ed6b7b1d21f2bd593d5ef9b477306c6d5c9be2399cacdcae98b507037f28` |
