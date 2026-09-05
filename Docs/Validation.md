# Validation records

Current Candidate05 behavior and verification are recorded in
[Candidate05](Passes/Candidate05.md) and its [evidence index](../Evidence/Candidate05/README.md).
Committed magazine reloads and non-buffered firing replace the older policy;
historical validation below applies only to the preserved Candidate01 build.

# Candidate 01 validation — 4 September 2026

Final gameplay source revision: `984bd90`. This report concerns the original Project ONE candidate, not Project Zero. The earlier exposure, inverted-map and capture-route probes under `Saved` are superseded. Use `Evidence/Packaged` for final standalone evidence.

## Implemented

One original response character, one modular infected archetype, one carbine and a compact 24×20m facility. Editable Blender rigs and 15 authored skeletal clips drive idle, walk/run, directional locomotion, firing, reload, infected attacks, hits and death. Upper-body action blending preserves moving legs. Native mouse aim is independent of movement.

The scene includes navigation around authored obstacles, timed attacks, ammunition/reload/empty states, muzzle/tracer/hit feedback, directional blood and persistent ground marks, intact deaths, head removal, and surviving arm removal with its own attack clip. Removed parts copy the evaluated pose and remain nonblocking. Caps: 32 short effects, 90 decals (150s), 18 pieces (18s), 14 corpses (28s). Rounds, health, points, pause, game over and restart are present. No multiplayer or wider progression systems were added.

## Built

UE 5.7.2 editor and Win64 Development game compiled successfully. BuildCookRun cooked, staged and archived the standalone `Packaged/Candidate01/Windows` folder successfully. Final cook reported zero errors and zero warnings. MSVC 14.50 is newer than UE's preferred 14.44 compiler; it was used successfully without replacing the installed toolchain.

Original pre-publication source/import audit: all 39 FBX/PNG/WAV hashes matched their imported Unreal assets. Before publication, author-path metadata was removed from 12 FBXs without changing their parsed geometry, animation or other properties. Their byte hashes therefore changed; the saved Candidate01 hash report is historical. Blender render/file-browser paths and PNG text metadata were also made portable; scene data and image pixels were verified unchanged. Skin weights, imported centimeter scale, axes, skeleton compatibility, clip lengths, material slots and vertex colors were inspected. Map checks verify upright walls, level floor collision and visible markings. See `Evidence/final_source_sync.json`, `unreal_asset_audit.json` and the pipeline notes.

## Automated tests passed

The Unreal Automation test `ProjectONE.Combat.DamageAndDeathAreMonotonic` passed all nine assertions (one test, zero failures). Raw machine-specific automation logs remain local; this summary and the focused runtime reports are public.

The packaged presentation driver passed 33 checks: idle, forward/back/sideways/diagonal movement, 180cm/s walk and 370cm/s run, foot pose changes, moving fire/reload, real aimed bullet damage to three enemies, evaluated modular bones, uninterrupted timed contact and surviving-arm attacks. The driver issued 91 capture requests; 90 PNGs were actually saved, spanning 12.388s (about 7.18fps). One request was superseded by another screenshot in the same engine frame. The GIF includes only those 90 existing frames. This measures recording throughput, not game performance. A first capture run failed an overstrict requested-frame-rate assertion; the final test verifies that timestamped requests span the intended movement and combat interval, and reports requested throughput separately from the file audit.

The packaged runtime harness passed all 34 checks: imported assets, current-pose arm detachment, surviving pursuit, duplicate sever/damage guards, head and intact deaths, held-trigger empty state, ammunition conservation, round two, duplicate rewards, canceled pending attacks, effect limits and timed cleanup, lethal damage during reload, game over and restart. These use the real gameplay code, with scripted test setup; they are not a substitute for human playtesting.

## Visually inspected

Genuine game-camera framebuffer images show idle, walk, run, backward/sideways movement, moving fire/reload, blood, intact and severed bodies, attack contact, crowds, round transitions and game over. A second reviewer inspected the source/import and final moving-fire, run and one-arm-contact images. The final FXAA image removes the conspicuous temporal trails in the preceding TAA version. Closer attack spacing improves hand-to-player contact.

`Evidence/ScriptedGameplay.gif` uses only genuine engine frames and their recorded timestamps. It is silent and scripted, with no generated or interpolated frames. PNG readback affects the recording run; separate benchmarks disable sequence capture. The GIF is useful for reviewing gross motion and combat but cannot establish perfect full-rate foot planting or animation feel.

## Packaged gameplay tested

The archived Win64 executable completed the presentation driver, all 34 runtime checks and the 6/12/18-enemy benchmarks. It then launched normally via `Scripts/Launch.ps1`, with no test flags and no Unreal Editor process running. Blender was not needed; an unrelated pre-existing unsaved Blender window was left alone.

Direct Windows UI input verified first-click firing (24→23 rounds), cursor-driven weapon turning, R reload (23/192→24/191), Escape pause/resume, actual unmodified enemy attacks causing game over, Enter restart from game over, and Enter restart from pause. A short Shift+W input was issued, but sustained gaits were evaluated with the scripted presentation driver. The game was left paused. `Evidence/Packaged/Manual` preserves actual window captures of pause, game over and restart. This was a bounded input check, not a completed human-style round or manual hit-region targeting audit.

## Performance

Hardware: RTX 3090, Ryzen 9 5950X, 64GiB RAM, Windows 11 25H2. Resolution: 1600×900 windowed, 100% screen percentage, DirectX 12 / SM6. View distance, shadows, post process, textures and effects use scalability level 2. FXAA, VSync off, 120fps cap; dynamic GI and reflection methods disabled. Full measured settings accompany each report.

Each benchmark warms up for 10 seconds, samples actual game frame deltas for 20 seconds, sustains the requested number of living pursuing/attacking enemies, and verifies that count before exiting. Player health is held up and enemy damage is zero only in this test. One screenshot is taken, but no sequence is recorded. These short capped runs do not establish uncapped headroom, minimum FPS or other-hardware guarantees.

| Actual active enemies | Samples | Mean frame time | Median | p95 |
| --- | --- | --- | --- | --- |
| 6 | 2,386 | 8.383 ms | 8.334 ms | 8.334 ms |
| 12 | 2,385 | 8.386 ms | 8.334 ms | 8.334 ms |
| 18 | 2,385 | 8.388 ms | 8.334 ms | 8.334 ms |

All three runs stayed near the 120fps cap on average (approximately 119fps). The percentile is below the mean because a few slower frames raise the average. Raw reports are `Evidence/Packaged/Validation/benchmark_6.txt`, `benchmark_12.txt` and `benchmark_18.txt`.

## Not yet verified / unfinished requirements

- The desired grounded production quality is not fully achieved. Faces, hands and clothing remain simplified; sprint poses look somewhat crouched, and attack silhouettes need stronger weight and contact. Art approval remains with the user.
- Detailed foot planting, aim-turn transitions and grip contact across every aim direction need full-rate human motion review. Measured foot movement and matching gait speeds are not proof of zero skating.
- FXAA trades the previous temporal trails for some aliasing on fine weapon edges and floor seams.
- Four original synthesized broadband/mechanical sounds are integrated. They are provisional, not recorded or finished realistic gunfire, and have not been perceptually auditioned by the assistant. No old sound assets or procedural beeps are presented as finished audio.
- Source left/right bone labels appear mirrored after FBX coordinate conversion. The modular contract is internally consistent; normalize the authoring axes, labels and handedness together in a later pipeline pass.
- Detached parts use bounded, collisionless ballistic motion with a frozen current pose, not full ragdoll simulation. They do not block movement or navigation.
- No long soak test, exhaustive navigation proof, accessibility/settings menu, other resolutions, other GPUs or clean-machine prerequisites installation has been verified. This is a local Win64 Development candidate.
- Runtime logs contain socket queries before meshes are assigned during spawn; final meshes and required bones load and animate successfully. There are no missing-asset claims based on those early warnings.

Next focused milestone: obtain the user's art-direction review, then refine this same character/animation/weapon-contact/audio set. Do not expand the map or roster before that decision.
